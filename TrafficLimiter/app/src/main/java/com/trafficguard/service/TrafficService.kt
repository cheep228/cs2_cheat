package com.trafficguard.service

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.content.pm.ServiceInfo
import android.net.TrafficStats
import android.net.VpnService
import android.os.Build
import android.os.Handler
import android.os.HandlerThread
import android.os.ParcelFileDescriptor
import android.os.SystemClock
import android.util.Log
import androidx.core.content.ContextCompat
import com.trafficguard.R
import com.trafficguard.data.BlockPlan
import com.trafficguard.data.CurrentNet
import com.trafficguard.data.Decision
import com.trafficguard.data.Engine
import com.trafficguard.data.Live
import com.trafficguard.data.LiveStatus
import com.trafficguard.data.NetKind
import com.trafficguard.data.Profile
import com.trafficguard.data.ProfileMatcher
import com.trafficguard.data.Quota
import com.trafficguard.data.SettingsRepo
import com.trafficguard.data.Usage
import com.trafficguard.data.UsageStore
import com.trafficguard.data.fmtBytes
import com.trafficguard.data.fmtDuration
import com.trafficguard.data.quotaName
import com.trafficguard.ui.MainActivity
import java.io.FileInputStream
import java.io.IOException
import java.util.concurrent.atomic.AtomicLong

/**
 * One foreground service does everything:
 *  - counts device traffic every second and books it to the profile of the current network;
 *  - decides which apps must lose network access;
 *  - blocks them with a local VPN: only blocked apps are routed into the tun interface,
 *    and every packet they send is dropped. Nothing leaves the phone, there is no server.
 */
class TrafficService : VpnService() {

    companion object {
        private const val TAG = "TrafficService"
        const val ACTION_START = "com.trafficguard.START"
        const val ACTION_STOP = "com.trafficguard.STOP"
        const val ACTION_TOGGLE_LIMIT = "com.trafficguard.TOGGLE_LIMIT"

        private const val CH_STATUS = "status"
        private const val CH_ALERT = "alerts"
        private const val NOTIF_ID = 1
        private const val ALERT_ID = 2

        fun start(ctx: Context) {
            ContextCompat.startForegroundService(
                ctx, Intent(ctx, TrafficService::class.java).setAction(ACTION_START),
            )
        }

        fun stop(ctx: Context) {
            if (!Live.status.value.running) return
            ctx.startService(Intent(ctx, TrafficService::class.java).setAction(ACTION_STOP))
        }
    }

    private lateinit var worker: HandlerThread
    private lateinit var handler: Handler
    private lateinit var network: NetworkWatcher
    private lateinit var foreground: ForegroundTracker

    private var tun: TunSink? = null
    private var plan: BlockPlan = BlockPlan.None
    private val dropped = AtomicLong()

    private var lastTotal = -1L
    private var lastDropped = 0L
    private var lastTickAt = 0L
    private var lastSaveAt = 0L
    private var appUsage: Map<String, Long> = emptyMap()
    private var appUsageAt = 0L
    private var appUsageKind: NetKind? = null
    private var lastExceeded: Quota? = null
    private var lastNotifText = ""
    private var lastNotifAt = 0L
    private var vpnPermissionMissing = false
    private var vpnError: String? = null
    private var stopping = false

    private val tick = object : Runnable {
        override fun run() {
            try {
                step()
            } catch (t: Throwable) {
                Log.e(TAG, "tick failed", t)
            }
            if (!stopping) handler.postDelayed(this, 1_000)
        }
    }

    override fun onCreate() {
        super.onCreate()
        createChannels()
        goForeground(buildNotification("Запуск…", "", null))
        network = NetworkWatcher(this).also { it.start() }
        foreground = ForegroundTracker(this)
        worker = HandlerThread("traffic").also { it.start() }
        handler = Handler(worker.looper)
        handler.post(tick)
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        when (intent?.action) {
            ACTION_STOP -> {
                stopping = true
                handler.post {
                    closeTun()
                    UsageStore.save()
                    stopForeground(STOP_FOREGROUND_REMOVE)
                    stopSelf()
                }
                return START_NOT_STICKY
            }
            ACTION_TOGGLE_LIMIT -> SettingsRepo.update { it.copy(limitingEnabled = !it.limitingEnabled) }
        }
        goForeground(buildNotification(lastNotifTitle(), lastNotifText, null))
        return START_STICKY
    }

    override fun onRevoke() {
        // Another VPN took over, or the user revoked us. Keep counting, stop blocking.
        handler.post {
            closeTun()
            plan = BlockPlan.None
        }
    }

    override fun onDestroy() {
        stopping = true
        if (::handler.isInitialized) handler.removeCallbacksAndMessages(null)
        closeTun()
        if (::network.isInitialized) network.stop()
        if (::worker.isInitialized) worker.quitSafely()
        UsageStore.save()
        Live.status.value = Live.status.value.copy(running = false)
        super.onDestroy()
    }

    // ------------------------------------------------------------------ main loop

    private fun step() {
        val now = System.currentTimeMillis()
        val nowRt = SystemClock.elapsedRealtime()
        val settings = SettingsRepo.settings.value
        val net = network.current()
        val profile = ProfileMatcher.match(settings.profiles, net)

        // Bytes moved since the last tick. Packets that blocked apps push into our
        // tun show up in the device total too, so the bytes we dropped are taken out.
        val rx = TrafficStats.getTotalRxBytes()
        val tx = TrafficStats.getTotalTxBytes()
        val total = if (rx < 0 || tx < 0) -1L else rx + tx
        val droppedNow = dropped.get()
        var delta = 0L
        if (total >= 0 && lastTotal >= 0) delta = (total - lastTotal) - (droppedNow - lastDropped)
        if (delta < 0) delta = 0
        lastTotal = total
        lastDropped = droppedNow
        val elapsed = if (lastTickAt == 0L) 1_000 else (nowRt - lastTickAt).coerceAtLeast(1)
        lastTickAt = nowRt
        val speed = delta * 1_000 / elapsed

        val usage: Usage? = profile?.let { UsageStore.add(it.id, delta, now) }

        val usageAccess = Perms.usageAccess(this)
        refreshAppUsage(profile, net, usageAccess, now)

        val watchForeground = settings.limitingEnabled && profile?.limitEnabled == true &&
            profile.blockBackground && usageAccess
        val fg = if (watchForeground) foreground.current(now) else null

        val decision = Engine.decide(
            limitingEnabled = settings.limitingEnabled,
            profile = profile,
            usage = usage,
            appUsageToday = appUsage,
            foreground = fg,
            foregroundKnown = watchForeground,
            ownPackage = packageName,
            now = now,
        )
        applyPlan(decision.plan)

        if (decision.exceeded != null && decision.exceeded != lastExceeded && profile != null) {
            alertExceeded(profile, decision.exceeded, now)
        }
        lastExceeded = decision.exceeded

        Live.status.value = LiveStatus(
            running = true,
            net = net,
            profileId = profile?.id,
            usage = usage,
            bytesPerSecond = speed,
            decision = decision,
            foreground = fg,
            vpnPermissionMissing = vpnPermissionMissing,
            vpnError = vpnError,
        )
        updateNotification(net, profile, usage, speed, decision, now)

        if (now - lastSaveAt > 15_000) {
            UsageStore.save()
            lastSaveAt = now
        }
    }

    private fun refreshAppUsage(profile: Profile?, net: CurrentNet, usageAccess: Boolean, now: Long) {
        val needed = profile != null && profile.apps.values.any { it.dailyLimit > 0 }
        if (!needed || !usageAccess) {
            appUsage = emptyMap()
            appUsageKind = null
            return
        }
        if (appUsageKind != net.kind || now - appUsageAt > 30_000) {
            appUsage = runCatching { NetStats.todayByPackage(this, net.kind, now) }.getOrDefault(emptyMap())
            appUsageAt = now
            appUsageKind = net.kind
        }
    }

    // ------------------------------------------------------------------ VPN

    private fun applyPlan(p: BlockPlan) {
        if (p == plan && (p == BlockPlan.None || tun != null)) return
        if (p == BlockPlan.None) {
            closeTun()
            plan = p
            vpnError = null
            return
        }
        if (prepare(this) != null) {
            vpnPermissionMissing = true
            closeTun()
            plan = BlockPlan.None
            return
        }
        vpnPermissionMissing = false

        val b = Builder()
            .setSession(getString(R.string.app_name))
            .addAddress("10.215.73.1", 32)
            .addRoute("0.0.0.0", 0)
            .addAddress("fd00:7a7a::1", 128)
            .addRoute("::", 0)
            .setMtu(1500)
            .setBlocking(false)
        when (p) {
            is BlockPlan.Only -> {
                var added = 0
                for (pkg in p.packages) {
                    if (pkg == packageName) continue
                    try {
                        b.addAllowedApplication(pkg)
                        added++
                    } catch (_: PackageManager.NameNotFoundException) {
                    }
                }
                if (added == 0) {
                    // Every blocked app is uninstalled: nothing to block.
                    closeTun()
                    plan = p
                    return
                }
            }
            is BlockPlan.AllExcept -> {
                b.addDisallowedApplication(packageName)
                for (pkg in p.packages) {
                    if (pkg == packageName) continue
                    try {
                        b.addDisallowedApplication(pkg)
                    } catch (_: PackageManager.NameNotFoundException) {
                    }
                }
            }
            BlockPlan.None -> Unit
        }

        val pfd = try {
            b.establish()
        } catch (e: Exception) {
            Log.e(TAG, "establish failed", e)
            null
        }
        if (pfd == null) {
            vpnError = "Не удалось включить VPN-блокировку"
            plan = BlockPlan.None
            return
        }
        vpnError = null
        val old = tun
        tun = TunSink(pfd, dropped).also { it.start() }
        old?.shutdown()
        plan = p
    }

    private fun closeTun() {
        tun?.shutdown()
        tun = null
    }

    /** Reads whatever blocked apps send into the tun and throws it away. */
    private class TunSink(
        private val pfd: ParcelFileDescriptor,
        private val counter: AtomicLong,
    ) : Thread("tun-sink") {
        @Volatile
        private var running = true

        override fun run() {
            val input = FileInputStream(pfd.fileDescriptor)
            val buf = ByteArray(32767)
            try {
                while (running) {
                    val n = try {
                        input.read(buf)
                    } catch (e: IOException) {
                        0 // EAGAIN on the non-blocking fd
                    }
                    when {
                        n > 0 -> counter.addAndGet(n.toLong())
                        n < 0 -> break
                        else -> sleep(25)
                    }
                }
            } catch (_: InterruptedException) {
            } finally {
                try {
                    pfd.close()
                } catch (_: IOException) {
                }
            }
        }

        fun shutdown() {
            running = false
            interrupt()
        }
    }

    // ------------------------------------------------------------------ notifications

    private fun createChannels() {
        val nm = getSystemService(NotificationManager::class.java)
        nm.createNotificationChannel(
            NotificationChannel(CH_STATUS, "Состояние", NotificationManager.IMPORTANCE_LOW).apply {
                setShowBadge(false)
            },
        )
        nm.createNotificationChannel(
            NotificationChannel(CH_ALERT, "Превышение лимита", NotificationManager.IMPORTANCE_HIGH),
        )
    }

    private fun goForeground(n: Notification) {
        if (Build.VERSION.SDK_INT >= 34) {
            startForeground(NOTIF_ID, n, ServiceInfo.FOREGROUND_SERVICE_TYPE_SPECIAL_USE)
        } else {
            startForeground(NOTIF_ID, n)
        }
    }

    private fun openAppIntent(): PendingIntent = PendingIntent.getActivity(
        this, 0, Intent(this, MainActivity::class.java).addFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP),
        PendingIntent.FLAG_IMMUTABLE or PendingIntent.FLAG_UPDATE_CURRENT,
    )

    private var notifTitle = "Лимит трафика"
    private fun lastNotifTitle() = notifTitle

    private fun buildNotification(title: String, text: String, decision: Decision?): Notification {
        notifTitle = title
        val limiting = SettingsRepo.settings.value.limitingEnabled
        val toggle = PendingIntent.getService(
            this, 1, Intent(this, TrafficService::class.java).setAction(ACTION_TOGGLE_LIMIT),
            PendingIntent.FLAG_IMMUTABLE or PendingIntent.FLAG_UPDATE_CURRENT,
        )
        return Notification.Builder(this, CH_STATUS)
            .setSmallIcon(R.drawable.ic_stat)
            .setContentTitle(title)
            .setContentText(text)
            .setStyle(Notification.BigTextStyle().bigText(text))
            .setContentIntent(openAppIntent())
            .setOngoing(true)
            .setOnlyAlertOnce(true)
            .setShowWhen(false)
            .addAction(
                Notification.Action.Builder(
                    null, if (limiting) "Выключить ограничение" else "Включить ограничение", toggle,
                ).build(),
            )
            .also { if (decision?.exceeded != null) it.setColor(0xFFD32F2F.toInt()) }
            .build()
    }

    private fun updateNotification(
        net: CurrentNet, profile: Profile?, usage: Usage?, speed: Long, d: Decision, now: Long,
    ) {
        val netName = when (net.kind) {
            NetKind.WIFI -> "Wi‑Fi " + (net.ssid?.let { "«$it»" } ?: "")
            NetKind.MOBILE -> "Мобильная сеть"
            NetKind.OTHER -> "Другая сеть"
            NetKind.NONE -> "Нет сети"
        }
        val title = when {
            d.exceeded != null -> "⛔ Лимит ${quotaName(d.exceeded)} исчерпан — интернет заблокирован"
            d.plan is BlockPlan.AllExcept -> "$netName · ${fmtBytes(speed)}/с · фон заблокирован"
            d.plan is BlockPlan.Only -> "$netName · ${fmtBytes(speed)}/с · блок: ${d.blockedApps.size} прил."
            else -> "$netName · ${fmtBytes(speed)}/с"
        }
        val text = buildString {
            if (profile != null && usage != null) {
                append("Минута ").append(part(usage.minute, profile.perMinute))
                append(" · Час ").append(part(usage.hour, profile.perHour))
                append(" · День ").append(part(usage.day, profile.perDay))
                if (profile.maxTotal > 0) append(" · Всего ").append(part(usage.total, profile.maxTotal))
                d.exceeded?.let { q ->
                    append("\n")
                    append(Engine.untilReset(q, now)?.let { "Разблокировка через ${fmtDuration(it)}" }
                        ?: "Сбросьте счётчик в приложении, чтобы разблокировать")
                }
            } else {
                append(d.note)
            }
            if (vpnPermissionMissing) append("\nНет разрешения VPN — блокировка не работает")
        }
        val t = SystemClock.elapsedRealtime()
        if (title == notifTitle && text == lastNotifText) return
        // Speed changes every second; repaint at most every 2 s unless the state changed.
        val stateChanged = (d.exceeded != null) != notifTitle.startsWith("⛔")
        if (!stateChanged && t - lastNotifAt < 2_000) return
        lastNotifAt = t
        lastNotifText = text
        getSystemService(NotificationManager::class.java).notify(NOTIF_ID, buildNotification(title, text, d))
    }

    private fun part(used: Long, limit: Long) =
        if (limit > 0) "${fmtBytes(used)}/${fmtBytes(limit)}" else fmtBytes(used)

    private fun alertExceeded(p: Profile, q: Quota, now: Long) {
        if (!Perms.notifications(this)) return
        val wait = Engine.untilReset(q, now)?.let { "Разблокировка через ${fmtDuration(it)}." }
            ?: "Сбросьте счётчик в приложении, чтобы разблокировать."
        val n = Notification.Builder(this, CH_ALERT)
            .setSmallIcon(R.drawable.ic_stat)
            .setContentTitle("Лимит ${quotaName(q)} исчерпан: ${p.name}")
            .setContentText("Весь трафик, кроме разрешённых приложений, заблокирован. $wait")
            .setStyle(Notification.BigTextStyle().bigText("Весь трафик, кроме разрешённых приложений, заблокирован. $wait"))
            .setContentIntent(openAppIntent())
            .setAutoCancel(true)
            .build()
        getSystemService(NotificationManager::class.java).notify(ALERT_ID, n)
    }
}
