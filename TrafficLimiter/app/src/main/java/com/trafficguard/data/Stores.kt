package com.trafficguard.data

import android.content.Context
import android.content.SharedPreferences
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import java.util.UUID

/** User settings, shared by the UI and the service (same process). */
object SettingsRepo {
    private lateinit var prefs: SharedPreferences
    private val state = MutableStateFlow(Settings())
    val settings: StateFlow<Settings> = state

    fun init(ctx: Context) {
        prefs = ctx.getSharedPreferences("settings", Context.MODE_PRIVATE)
        val saved = prefs.getString("settings", null)
        state.value = saved?.let { runCatching { settingsFromJson(it) }.getOrNull() } ?: defaults()
        if (saved == null) persist(state.value)
    }

    private fun defaults() = Settings(
        profiles = listOf(
            Profile(newId(), "Мобильная сеть", NetType.MOBILE),
            Profile(newId(), "Любой Wi‑Fi", NetType.WIFI),
        ),
    )

    fun newId(): String = UUID.randomUUID().toString()

    @Synchronized
    fun update(transform: (Settings) -> Settings) {
        val next = transform(state.value)
        if (next == state.value) return
        state.value = next
        persist(next)
    }

    fun updateProfile(id: String, transform: (Profile) -> Profile) = update { s ->
        s.copy(profiles = s.profiles.map { if (it.id == id) transform(it) else it })
    }

    private fun persist(s: Settings) {
        prefs.edit().putString("settings", s.toJson()).apply()
    }
}

/** Traffic counters per profile id. */
object UsageStore {
    private lateinit var prefs: SharedPreferences
    private val map = HashMap<String, Usage>()

    @Synchronized
    fun init(ctx: Context) {
        prefs = ctx.getSharedPreferences("usage", Context.MODE_PRIVATE)
        prefs.all.forEach { (k, v) ->
            (v as? String)?.let { runCatching { Usage.fromJson(it) }.getOrNull() }?.let { map[k] = it }
        }
    }

    @Synchronized
    fun get(id: String, now: Long): Usage = (map[id] ?: Usage(totalSince = now)).rolled(now)

    @Synchronized
    fun add(id: String, bytes: Long, now: Long): Usage {
        val u = (map[id] ?: Usage(totalSince = now)).plus(bytes, now)
        map[id] = u
        return u
    }

    @Synchronized
    fun reset(id: String, now: Long) {
        map[id] = Usage(totalSince = now).rolled(now)
        save()
    }

    @Synchronized
    fun remove(id: String) {
        map.remove(id)
        save()
    }

    @Synchronized
    fun save() {
        val e = prefs.edit().clear()
        map.forEach { (k, v) -> e.putString(k, v.toJson()) }
        e.apply()
    }
}

/** What the service is doing right now, for the UI. */
data class LiveStatus(
    val running: Boolean = false,
    val net: CurrentNet = CurrentNet(NetKind.NONE),
    val profileId: String? = null,
    val usage: Usage? = null,
    val bytesPerSecond: Long = 0,
    val decision: Decision? = null,
    val foreground: String? = null,
    val vpnPermissionMissing: Boolean = false,
    val vpnError: String? = null,
    /** True when blocking goes through root iptables rather than the VPN. */
    val viaRoot: Boolean = false,
    val rootError: String? = null,
)

object Live {
    val status = MutableStateFlow(LiveStatus())
}
