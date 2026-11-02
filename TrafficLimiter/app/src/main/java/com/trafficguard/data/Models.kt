package com.trafficguard.data

import org.json.JSONArray
import org.json.JSONObject

/** Which network a profile applies to. */
enum class NetType { WIFI, MOBILE, ANY }

/** How a single app is treated inside a profile. */
enum class AppMode {
    /** Subject to the network's limits and to its own daily limit, if set. */
    NORMAL,

    /** Never gets traffic on this network. */
    BLOCK,

    /** Never blocked, even when the network's limit is exhausted. */
    ALLOW,
}

data class AppRule(
    val mode: AppMode = AppMode.NORMAL,
    /** Bytes per day this app may use on this network; 0 means no per-app limit. */
    val dailyLimit: Long = 0L,
) {
    val isDefault: Boolean get() = mode == AppMode.NORMAL && dailyLimit <= 0
}

/**
 * Rules for one network: a specific Wi-Fi (by SSID), any Wi-Fi, mobile data,
 * or any network. All limits are in bytes; 0 means "no limit".
 */
data class Profile(
    val id: String,
    val name: String,
    val type: NetType,
    /** Only for [NetType.WIFI]; blank means "any Wi-Fi". */
    val ssid: String = "",
    /** The per-network "limit traffic or not" checkbox. */
    val limitEnabled: Boolean = false,
    val perMinute: Long = 0L,
    val perHour: Long = 0L,
    val perDay: Long = 0L,
    /** Max total until the counter is reset by hand. */
    val maxTotal: Long = 0L,
    /** Only the app on screen (and ALLOW apps) gets traffic. */
    val blockBackground: Boolean = false,
    val apps: Map<String, AppRule> = emptyMap(),
) {
    fun rule(pkg: String): AppRule = apps[pkg] ?: AppRule()
}

data class Settings(
    /** Master "limit traffic" checkbox. Off = count only, never block. */
    val limitingEnabled: Boolean = false,
    /** Keep the background service running (counting + enforcing). */
    val monitorEnabled: Boolean = true,
    /** Block with iptables through root instead of the local VPN. */
    val useRoot: Boolean = false,
    val profiles: List<Profile> = emptyList(),
)

// ---------------------------------------------------------------- JSON

fun Settings.toJson(): String = JSONObject().apply {
    put("limitingEnabled", limitingEnabled)
    put("monitorEnabled", monitorEnabled)
    put("useRoot", useRoot)
    put("profiles", JSONArray().apply { profiles.forEach { put(it.toJsonObject()) } })
}.toString()

fun settingsFromJson(text: String): Settings {
    val o = JSONObject(text)
    val arr = o.optJSONArray("profiles") ?: JSONArray()
    val profiles = (0 until arr.length()).mapNotNull { i ->
        runCatching { profileFromJson(arr.getJSONObject(i)) }.getOrNull()
    }
    return Settings(
        limitingEnabled = o.optBoolean("limitingEnabled", false),
        monitorEnabled = o.optBoolean("monitorEnabled", true),
        useRoot = o.optBoolean("useRoot", false),
        profiles = profiles,
    )
}

private fun Profile.toJsonObject(): JSONObject = JSONObject().apply {
    put("id", id)
    put("name", name)
    put("type", type.name)
    put("ssid", ssid)
    put("limitEnabled", limitEnabled)
    put("perMinute", perMinute)
    put("perHour", perHour)
    put("perDay", perDay)
    put("maxTotal", maxTotal)
    put("blockBackground", blockBackground)
    put("apps", JSONObject().apply {
        apps.forEach { (pkg, r) ->
            put(pkg, JSONObject().put("mode", r.mode.name).put("dailyLimit", r.dailyLimit))
        }
    })
}

private fun profileFromJson(o: JSONObject): Profile {
    val appsObj = o.optJSONObject("apps") ?: JSONObject()
    val apps = buildMap {
        appsObj.keys().forEach { pkg ->
            val r = appsObj.getJSONObject(pkg)
            val mode = runCatching { AppMode.valueOf(r.optString("mode")) }.getOrDefault(AppMode.NORMAL)
            put(pkg, AppRule(mode, r.optLong("dailyLimit", 0L)))
        }
    }
    return Profile(
        id = o.getString("id"),
        name = o.optString("name", ""),
        type = runCatching { NetType.valueOf(o.optString("type")) }.getOrDefault(NetType.ANY),
        ssid = o.optString("ssid", ""),
        limitEnabled = o.optBoolean("limitEnabled", false),
        perMinute = o.optLong("perMinute", 0L),
        perHour = o.optLong("perHour", 0L),
        perDay = o.optLong("perDay", 0L),
        maxTotal = o.optLong("maxTotal", 0L),
        blockBackground = o.optBoolean("blockBackground", false),
        apps = apps,
    )
}
