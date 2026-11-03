package com.niri.recents.hook

import android.content.Context
import android.net.Uri
import android.os.Bundle
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

    /** Version of the wallpaper copy the settings app holds (0 = none yet). */
    @Volatile
    var wallpaperStamp = 0L

    fun refresh(context: Context, force: Boolean = false) {
        val now = SystemClock.uptimeMillis()
        if (!force && now - lastRefresh < 4_000) return
        lastRefresh = now
        val app = context.applicationContext ?: context
        Bg.handler.post {
            try {
                // Hands the tiling desktop its command channel at the same time.
                val extras = Bundle().apply { putBinder(SettingsContract.KEY_TILER_BINDER, NiriTiler.binder) }
                val b = app.contentResolver.call(uri, SettingsContract.METHOD_GET, null, extras)
                if (b != null) {
                    cached = NiriSettings.fromBundle(b)
                    wallpaperStamp = b.getLong(SettingsContract.KEY_WALLPAPER_STAMP, 0L)
                }
            } catch (t: Throwable) {
                // Settings app missing or not reachable: keep defaults.
            }
        }
    }

    /** The real wallpaper copied by the settings app. Blocking. */
    fun loadWallpaper(context: Context, width: Int): android.graphics.Bitmap? = try {
        val app = context.applicationContext ?: context
        val bytes = app.contentResolver.openInputStream(Uri.parse(SettingsContract.WALLPAPER_URI))?.use { it.readBytes() }
        bytes?.let {
            val o = android.graphics.BitmapFactory.Options().apply { inJustDecodeBounds = true }
            android.graphics.BitmapFactory.decodeByteArray(it, 0, it.size, o)
            var sample = 1
            while (o.outWidth / (sample * 2) >= width) sample *= 2
            android.graphics.BitmapFactory.decodeByteArray(it, 0, it.size, android.graphics.BitmapFactory.Options().apply { inSampleSize = sample })
        }
    } catch (t: Throwable) {
        null
    }

    /** Saves a pinched window size in the settings app. */
    fun setZoom(context: Context, zoom: Float) {
        cached = cached?.copy(zoom = zoom)
        val app = context.applicationContext ?: context
        Bg.handler.post {
            try {
                app.contentResolver.call(uri, SettingsContract.METHOD_SET_ZOOM, zoom.toString(), null)
            } catch (t: Throwable) {
                // Settings app missing: the size lasts until the launcher restarts.
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

    /** Tiling column widths (fraction of the screen) by column key. */
    fun widths(): MutableMap<String, Float> = try {
        val o = org.json.JSONObject(prefs?.getString(K_WIDTHS, null) ?: "{}")
        o.keys().asSequence().associateWith { o.getDouble(it).toFloat() }.toMutableMap()
    } catch (t: Throwable) {
        mutableMapOf()
    }

    fun saveWidths(widths: Map<String, Float>) {
        try {
            val o = org.json.JSONObject()
            for ((k, v) in widths) o.put(k, v.toDouble())
            prefs?.edit()?.putString(K_WIDTHS, o.toString())?.apply()
        } catch (t: Throwable) {
            // Ignore.
        }
    }

    /** Tasks the tiler turned into windows, so they can be given back even after a restart. */
    fun tiledTasks(): Set<Int> = try {
        prefs?.getString(K_TILED, null)?.split(',')?.mapNotNull { it.toIntOrNull() }?.toSet() ?: emptySet()
    } catch (t: Throwable) {
        emptySet()
    }

    fun saveTiledTasks(ids: Set<Int>) {
        try {
            prefs?.edit()?.putString(K_TILED, ids.joinToString(","))?.apply()
        } catch (t: Throwable) {
            // Ignore.
        }
    }

    private companion object {
        // "state2": 1.3 lays new apps out over several workspaces; older layouts start over once.
        const val K_STATE = "state2"
        const val K_RESET = "reset"
        const val K_WIDTHS = "widths"
        const val K_TILED = "tiled"
    }
}
