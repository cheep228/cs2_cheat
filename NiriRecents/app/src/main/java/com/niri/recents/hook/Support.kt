package com.niri.recents.hook

import android.content.Context
import android.net.Uri
import android.os.Handler
import android.os.HandlerThread
import android.os.SystemClock
import com.niri.recents.overview.WsJson
import com.niri.recents.overview.WsState
import com.niri.recents.shared.NiriSettings
import com.niri.recents.shared.SettingsContract
import de.robv.android.xposed.XposedBridge
import kotlin.math.max

/** One background thread for snapshots, wallpaper and IPC. */
internal object Bg {
    val handler: Handler by lazy {
        val t = HandlerThread("niri-recents")
        t.start()
        Handler(t.looper)
    }
}

/** Talks to the settings app: reads settings, sends diagnostics the user can see there. */
internal object Remote {
    private val uri: Uri = Uri.parse(SettingsContract.URI)

    @Volatile
    var cached: NiriSettings? = null

    @Volatile
    private var lastRefresh = 0L

    fun refresh(context: Context, force: Boolean = false) {
        val now = SystemClock.uptimeMillis()
        if (!force && now - lastRefresh < 4_000) return
        lastRefresh = now
        val app = context.applicationContext ?: context
        Bg.handler.post {
            try {
                val b = app.contentResolver.call(uri, SettingsContract.METHOD_GET, null, null)
                if (b != null) cached = NiriSettings.fromBundle(b)
            } catch (t: Throwable) {
                // Settings app missing or not reachable: keep defaults.
            }
        }
    }

    fun report(context: Context, message: String) {
        try {
            XposedBridge.log("NiriRecents: $message")
        } catch (t: Throwable) {
            // Not running under Xposed.
        }
        val app = context.applicationContext ?: context
        Bg.handler.post {
            try {
                app.contentResolver.call(uri, SettingsContract.METHOD_REPORT, message, null)
            } catch (t: Throwable) {
                // Ignore.
            }
        }
    }
}

/** Workspace layout, saved in the launcher's own data (it belongs to the launcher's tasks). */
internal class WorkspaceStore(context: Context) {
    private val prefs = try {
        context.getSharedPreferences("niri_recents_workspaces", Context.MODE_PRIVATE)
    } catch (t: Throwable) {
        null // credential storage locked (before first unlock)
    }

    fun load(): WsState? = try {
        prefs?.getString(K_STATE, null)?.let { WsJson.decode(it) }
    } catch (t: Throwable) {
        null
    }

    fun save(state: WsState) {
        try {
            prefs?.edit()?.putString(K_STATE, WsJson.encode(state))?.apply()
        } catch (t: Throwable) {
            // Ignore.
        }
    }

    fun saveLayout(spaces: List<Pair<Int, List<String>>>, activeId: Int) {
        val old = load()
        val nextId = maxOf(old?.nextId ?: 1, (spaces.maxOfOrNull { it.first } ?: 0) + 1, activeId + 1)
        save(WsState(spaces.map { it.first }, spaces.toMap(), activeId, nextId))
    }

    fun setLastActive(id: Int) {
        val old = load() ?: WsState(emptyList(), emptyMap(), id, id + 1)
        save(old.copy(lastActive = id, nextId = max(old.nextId, id + 1)))
    }

    var resetToken: Long
        get() = try {
            prefs?.getLong(K_RESET, 0L) ?: 0L
        } catch (t: Throwable) {
            0L
        }
        set(v) {
            try {
                prefs?.edit()?.putLong(K_RESET, v)?.apply()
            } catch (t: Throwable) {
                // Ignore.
            }
        }

    fun clear() {
        try {
            prefs?.edit()?.remove(K_STATE)?.apply()
        } catch (t: Throwable) {
            // Ignore.
        }
    }

    private companion object {
        const val K_STATE = "state"
        const val K_RESET = "reset"
    }
}
