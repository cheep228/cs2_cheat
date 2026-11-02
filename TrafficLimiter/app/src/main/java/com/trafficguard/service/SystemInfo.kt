package com.trafficguard.service

import android.Manifest
import android.app.AppOpsManager
import android.app.usage.NetworkStats
import android.app.usage.NetworkStatsManager
import android.app.usage.UsageEvents
import android.app.usage.UsageStatsManager
import android.content.Context
import android.content.pm.ApplicationInfo
import android.content.pm.PackageManager
import android.location.LocationManager
import android.net.ConnectivityManager
import android.net.Network
import android.net.NetworkCapabilities
import android.net.NetworkRequest
import android.net.VpnService
import android.net.wifi.WifiInfo
import android.net.wifi.WifiManager
import android.os.Build
import android.os.PowerManager
import android.os.Process
import com.trafficguard.data.CurrentNet
import com.trafficguard.data.NetKind
import com.trafficguard.data.TimeKeys

object Perms {
    fun vpn(ctx: Context) = VpnService.prepare(ctx) == null

    fun notifications(ctx: Context) = Build.VERSION.SDK_INT < 33 ||
        ctx.checkSelfPermission(Manifest.permission.POST_NOTIFICATIONS) == PackageManager.PERMISSION_GRANTED

    fun location(ctx: Context) =
        ctx.checkSelfPermission(Manifest.permission.ACCESS_FINE_LOCATION) == PackageManager.PERMISSION_GRANTED

    fun backgroundLocation(ctx: Context) =
        ctx.checkSelfPermission(Manifest.permission.ACCESS_BACKGROUND_LOCATION) == PackageManager.PERMISSION_GRANTED

    fun locationServiceOn(ctx: Context) =
        (ctx.getSystemService(Context.LOCATION_SERVICE) as LocationManager).isLocationEnabled

    fun usageAccess(ctx: Context): Boolean {
        val ops = ctx.getSystemService(Context.APP_OPS_SERVICE) as AppOpsManager
        return ops.unsafeCheckOpNoThrow(
            AppOpsManager.OPSTR_GET_USAGE_STATS, Process.myUid(), ctx.packageName,
        ) == AppOpsManager.MODE_ALLOWED
    }

    fun ignoresBatteryOptimizations(ctx: Context) =
        (ctx.getSystemService(Context.POWER_SERVICE) as PowerManager).isIgnoringBatteryOptimizations(ctx.packageName)
}

private fun cleanSsid(raw: String?): String? {
    val s = raw?.trim()?.removeSurrounding("\"") ?: return null
    return if (s.isBlank() || s == WifiManager.UNKNOWN_SSID || s == "<unknown ssid>") null else s
}

/** Reports the real (non-VPN) network the phone is on and the Wi-Fi name. */
class NetworkWatcher(ctx: Context) {
    private val app = ctx.applicationContext
    private val cm = app.getSystemService(ConnectivityManager::class.java)
    private val wm = app.getSystemService(WifiManager::class.java)

    @Volatile
    private var cachedSsid: String? = null

    private val callback: ConnectivityManager.NetworkCallback =
        if (Build.VERSION.SDK_INT >= 31) {
            object : ConnectivityManager.NetworkCallback(FLAG_INCLUDE_LOCATION_INFO) {
                override fun onCapabilitiesChanged(network: Network, caps: NetworkCapabilities) {
                    cleanSsid((caps.transportInfo as? WifiInfo)?.ssid)?.let { cachedSsid = it }
                }

                override fun onLost(network: Network) {
                    cachedSsid = null
                }
            }
        } else {
            object : ConnectivityManager.NetworkCallback() {
                override fun onLost(network: Network) {
                    cachedSsid = null
                }
            }
        }

    fun start() {
        runCatching {
            cm.registerNetworkCallback(
                NetworkRequest.Builder().addTransportType(NetworkCapabilities.TRANSPORT_WIFI).build(),
                callback,
            )
        }
    }

    fun stop() {
        runCatching { cm.unregisterNetworkCallback(callback) }
    }

    fun current(): CurrentNet {
        var caps = cm.activeNetwork?.let { cm.getNetworkCapabilities(it) }
        if (caps == null || caps.hasTransport(NetworkCapabilities.TRANSPORT_VPN)) {
            @Suppress("DEPRECATION")
            caps = cm.allNetworks.asSequence()
                .mapNotNull { cm.getNetworkCapabilities(it) }
                .filter { !it.hasTransport(NetworkCapabilities.TRANSPORT_VPN) }
                .filter { it.hasCapability(NetworkCapabilities.NET_CAPABILITY_INTERNET) }
                .sortedByDescending { it.hasTransport(NetworkCapabilities.TRANSPORT_WIFI) }
                .firstOrNull()
        }
        return when {
            caps == null -> CurrentNet(NetKind.NONE)
            caps.hasTransport(NetworkCapabilities.TRANSPORT_WIFI) -> CurrentNet(NetKind.WIFI, ssid(caps))
            caps.hasTransport(NetworkCapabilities.TRANSPORT_CELLULAR) -> CurrentNet(NetKind.MOBILE)
            else -> CurrentNet(NetKind.OTHER)
        }
    }

    private fun ssid(caps: NetworkCapabilities): String? {
        cleanSsid((caps.transportInfo as? WifiInfo)?.ssid)?.let { return it }
        @Suppress("DEPRECATION")
        cleanSsid(runCatching { wm.connectionInfo?.ssid }.getOrNull())?.let { return it }
        return cachedSsid
    }

    companion object {
        /** One-shot lookup for the UI ("use the current Wi-Fi"). */
        fun currentSsid(ctx: Context): String? {
            val w = NetworkWatcher(ctx)
            val n = w.current()
            return if (n.kind == NetKind.WIFI) n.ssid else null
        }
    }
}

/** Tracks which app is on screen, from usage events. Null when screen is off/locked. */
class ForegroundTracker(ctx: Context) {
    private val app = ctx.applicationContext
    private val usm = app.getSystemService(UsageStatsManager::class.java)
    private var lastQuery = 0L
    private var fg: String? = null

    fun current(now: Long): String? {
        val from = if (lastQuery == 0L) now - TimeKeys.HOUR else lastQuery - 2_000
        val events = runCatching { usm.queryEvents(from, now) }.getOrNull() ?: return fg
        val e = UsageEvents.Event()
        while (events.hasNextEvent()) {
            events.getNextEvent(e)
            when (e.eventType) {
                UsageEvents.Event.ACTIVITY_RESUMED -> fg = e.packageName
                UsageEvents.Event.SCREEN_NON_INTERACTIVE,
                UsageEvents.Event.KEYGUARD_SHOWN -> fg = null
            }
        }
        lastQuery = now
        return fg
    }
}

/** Per-app traffic from the system's own accounting (needs usage access). */
object NetStats {
    fun todayByPackage(ctx: Context, kind: NetKind, now: Long = System.currentTimeMillis()): Map<String, Long> {
        val types = when (kind) {
            NetKind.WIFI -> listOf(ConnectivityManager.TYPE_WIFI)
            NetKind.MOBILE -> listOf(ConnectivityManager.TYPE_MOBILE)
            else -> listOf(ConnectivityManager.TYPE_WIFI, ConnectivityManager.TYPE_MOBILE)
        }
        val nsm = ctx.getSystemService(NetworkStatsManager::class.java)
        val pm = ctx.packageManager
        val byUid = HashMap<Int, Long>()
        val start = TimeKeys.startOfDay(now)
        for (t in types) {
            val stats = try {
                @Suppress("DEPRECATION")
                nsm.querySummary(t, null, start, now + 60_000)
            } catch (e: Exception) {
                continue
            }
            val b = NetworkStats.Bucket()
            while (stats.hasNextBucket()) {
                stats.getNextBucket(b)
                byUid[b.uid] = (byUid[b.uid] ?: 0L) + b.rxBytes + b.txBytes
            }
            stats.close()
        }
        val out = HashMap<String, Long>()
        byUid.forEach { (uid, bytes) ->
            pm.getPackagesForUid(uid)?.forEach { pkg -> out[pkg] = (out[pkg] ?: 0L) + bytes }
        }
        return out
    }
}

data class AppInfo(val pkg: String, val label: String, val system: Boolean)

object AppList {
    fun load(ctx: Context): List<AppInfo> {
        val pm = ctx.packageManager
        val pkgs = if (Build.VERSION.SDK_INT >= 33) {
            pm.getInstalledPackages(PackageManager.PackageInfoFlags.of(PackageManager.GET_PERMISSIONS.toLong()))
        } else {
            @Suppress("DEPRECATION")
            pm.getInstalledPackages(PackageManager.GET_PERMISSIONS)
        }
        return pkgs.asSequence()
            .filter { it.packageName != ctx.packageName }
            .filter { it.requestedPermissions?.contains(Manifest.permission.INTERNET) == true }
            .mapNotNull { pi ->
                val ai = pi.applicationInfo ?: return@mapNotNull null
                val system = ai.flags and ApplicationInfo.FLAG_SYSTEM != 0 &&
                    ai.flags and ApplicationInfo.FLAG_UPDATED_SYSTEM_APP == 0
                AppInfo(pi.packageName, ai.loadLabel(pm).toString(), system)
            }
            .sortedBy { it.label.lowercase() }
            .toList()
    }
}
