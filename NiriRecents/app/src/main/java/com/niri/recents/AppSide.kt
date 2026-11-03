package com.niri.recents

import android.content.ComponentName
import android.content.ContentProvider
import android.content.ContentValues
import android.content.Context
import android.content.Intent
import android.content.pm.ApplicationInfo
import android.content.res.Resources
import android.database.Cursor
import android.net.Uri
import android.os.Binder
import android.os.Bundle
import android.os.ParcelFileDescriptor
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import java.io.File
import com.niri.recents.shared.NiriSettings
import com.niri.recents.shared.SettingsContract
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale
import java.util.concurrent.TimeUnit
import kotlin.math.roundToInt

/** Replaced with `true` by the module when Vector/LSPosed loads it into this app. */
object ModuleStatus {
    @JvmStatic
    fun isActive(): Boolean = false
}

/** Settings as the user set them in this app. */
object SettingsStore {
    /** 2: sizes became on-screen sizes for the phone layout; older ones are dropped. */
    private const val VERSION = 2

    private fun prefs(c: Context) = c.getSharedPreferences("niri_settings", Context.MODE_PRIVATE)

    fun load(c: Context): NiriSettings {
        val p = prefs(c)
        val d = NiriSettings()
        val sizes = p.getInt("ver", 1) >= VERSION
        fun size(key: String, def: Float) = if (sizes) p.getFloat(key, def) else def
        return NiriSettings(
            enabled = p.getBoolean("enabled", d.enabled),
            zoom = size("zoom", d.zoom),
            gapsDp = size("gaps", d.gapsDp),
            workspaceGapDp = size("wsGap", d.workspaceGapDp),
            cornerDp = size("corner", d.cornerDp),
            ringWidthDp = size("ring", d.ringWidthDp),
            ringActive = p.getInt("ringActive", d.ringActive),
            ringInactive = p.getInt("ringInactive", d.ringInactive),
            backdrop = p.getInt("backdrop", d.backdrop),
            workspaceColor = p.getInt("wsColor", d.workspaceColor),
            wallpaper = p.getBoolean("wallpaper", d.wallpaper),
            shadows = p.getBoolean("shadows", d.shadows),
            labels = p.getBoolean("labels", d.labels),
            animSpeed = p.getFloat("speed", d.animSpeed),
            resetToken = p.getLong("reset", d.resetToken),
            md3 = p.getBoolean("md3", d.md3),
            perWorkspace = p.getInt("perWs", d.perWorkspace),
            tiling = p.getBoolean("tiling", d.tiling),
            columnWidth = p.getFloat("colWidth", d.columnWidth),
            hypr = p.getBoolean("hypr", d.hypr),
        )
    }

    fun save(c: Context, s: NiriSettings) {
        prefs(c).edit()
            .putInt("ver", VERSION)
            .putBoolean("enabled", s.enabled)
            .putFloat("zoom", s.zoom)
            .putFloat("gaps", s.gapsDp)
            .putFloat("wsGap", s.workspaceGapDp)
            .putFloat("corner", s.cornerDp)
            .putFloat("ring", s.ringWidthDp)
            .putInt("ringActive", s.ringActive)
            .putInt("ringInactive", s.ringInactive)
            .putInt("backdrop", s.backdrop)
            .putInt("wsColor", s.workspaceColor)
            .putBoolean("wallpaper", s.wallpaper)
            .putBoolean("shadows", s.shadows)
            .putBoolean("labels", s.labels)
            .putFloat("speed", s.animSpeed)
            .putLong("reset", s.resetToken)
            .putBoolean("md3", s.md3)
            .putInt("perWs", s.perWorkspace)
            .putBoolean("tiling", s.tiling)
            .putFloat("colWidth", s.columnWidth)
            .putBoolean("hypr", s.hypr)
            .apply()
    }
}

/** What the hook in the launcher reported: proof it runs, and errors to send back. */
object DiagStore {
    private const val MAX = 80
    private fun prefs(c: Context) = c.getSharedPreferences("niri_diag", Context.MODE_PRIVATE)

    @Synchronized
    fun add(c: Context, line: String) {
        val time = SimpleDateFormat("dd.MM HH:mm:ss", Locale.US).format(Date())
        val lines = lines(c).takeLast(MAX - 1) + "$time  $line"
        prefs(c).edit().putString("lines", lines.joinToString("\n")).putLong("last", System.currentTimeMillis()).commit()
    }

    fun lines(c: Context): List<String> =
        prefs(c).getString("lines", null)?.split("\n")?.filter { it.isNotBlank() } ?: emptyList()

    fun lastReport(c: Context): Long = prefs(c).getLong("last", 0L)

    fun clear(c: Context) {
        prefs(c).edit().clear().commit()
    }
}

/**
 * Exported so the launcher process can read settings and post diagnostics.
 * Reading settings is harmless; reports are accepted only from system apps.
 */
class SettingsProvider : ContentProvider() {
    override fun onCreate() = true

    override fun call(method: String, arg: String?, extras: Bundle?): Bundle? {
        val c = context ?: return null
        return when (method) {
            SettingsContract.METHOD_GET -> {
                if (callerIsSystemApp(c)) TilerLink.offerBinder(extras?.getBinder(SettingsContract.KEY_TILER_BINDER))
                SettingsStore.load(c).toBundle().apply {
                    val f = WallpaperSync.file(c)
                    putLong(SettingsContract.KEY_WALLPAPER_STAMP, if (f.exists()) f.lastModified() else 0L)
                }
            }
            SettingsContract.METHOD_REPORT -> {
                if (callerIsSystemApp(c) && arg != null) DiagStore.add(c, arg.take(1500))
                null
            }
            SettingsContract.METHOD_SET_ZOOM -> {
                val z = arg?.toFloatOrNull()
                if (callerIsSystemApp(c) && z != null) {
                    SettingsStore.save(c, SettingsStore.load(c).copy(zoom = (z * 100).roundToInt() / 100f))
                }
                null
            }
            SettingsContract.METHOD_TILER_STATE -> {
                if (callerIsSystemApp(c) && extras != null) {
                    TilerLink.offerBinder(extras.getBinder(SettingsContract.KEY_TILER_BINDER))
                    TilerLink.update(Bundle(extras).apply { remove(SettingsContract.KEY_TILER_BINDER) })
                }
                null
            }
            else -> null
        }
    }

    private fun callerIsSystemApp(c: Context): Boolean {
        val uid = Binder.getCallingUid()
        val pm = c.packageManager
        return pm.getPackagesForUid(uid).orEmpty().any { pkg ->
            try {
                pm.getApplicationInfo(pkg, 0).flags and ApplicationInfo.FLAG_SYSTEM != 0
            } catch (t: Throwable) {
                false
            }
        }
    }

    override fun openFile(uri: Uri, mode: String): ParcelFileDescriptor? {
        val c = context ?: return null
        if (uri.lastPathSegment != "wallpaper" || !callerIsSystemApp(c)) return null
        val f = WallpaperSync.file(c)
        return if (f.exists()) ParcelFileDescriptor.open(f, ParcelFileDescriptor.MODE_READ_ONLY) else null
    }

    override fun query(uri: Uri, p: Array<out String>?, s: String?, a: Array<out String>?, o: String?): Cursor? = null
    override fun getType(uri: Uri): String? = null
    override fun insert(uri: Uri, values: ContentValues?): Uri? = null
    override fun delete(uri: Uri, s: String?, a: Array<out String>?) = 0
    override fun update(uri: Uri, v: ContentValues?, s: String?, a: Array<out String>?) = 0
}

object RecentsInfo {
    /** The package that draws Android's Recents screen (normally the stock launcher). */
    fun recentsPackage(c: Context): String? {
        try {
            val res = Resources.getSystem()
            val id = res.getIdentifier("config_recentsComponentName", "string", "android")
            if (id != 0) {
                ComponentName.unflattenFromString(res.getString(id))?.packageName?.let { return it }
            }
        } catch (t: Throwable) {
            // Fall through.
        }
        val home = Intent(Intent.ACTION_MAIN).addCategory(Intent.CATEGORY_HOME)
        return c.packageManager.resolveActivity(home, 0)?.activityInfo?.packageName
    }
}

/**
 * The real system wallpaper. Since Android 14 apps without READ_WALLPAPER_INTERNAL (the launcher
 * among them) get the stock default wallpaper instead, so it is copied here with root and
 * handed to the launcher through [SettingsProvider].
 */
object WallpaperSync {
    private val sources = listOf("/data/system/users/0/wallpaper", "/data/system/users/0/wallpaper_orig")

    fun file(c: Context) = File(c.filesDir, "wallpaper.jpg")

    /** Returns null on success, or a reason. Blocking; call off the main thread. */
    fun sync(c: Context): String? {
        for (path in sources) {
            val bytes = RootShell.readBytes(path) ?: continue
            if (bytes.size < 1024) continue
            val bmp = decodeScaled(bytes, 1080) ?: continue
            val tmp = File(c.filesDir, "wallpaper.tmp")
            tmp.outputStream().use { bmp.compress(Bitmap.CompressFormat.JPEG, 90, it) }
            if (!tmp.renameTo(file(c))) return "не удалось сохранить"
            return null
        }
        return "нет доступа к $sources (root?) или стоят живые обои"
    }

    fun load(c: Context, width: Int): Bitmap? {
        val f = file(c)
        if (!f.exists()) return null
        return decodeScaled(f.readBytes(), width)
    }

    fun decodeScaled(bytes: ByteArray, width: Int): Bitmap? {
        val o = BitmapFactory.Options().apply { inJustDecodeBounds = true }
        BitmapFactory.decodeByteArray(bytes, 0, bytes.size, o)
        if (o.outWidth <= 0) return null
        var sample = 1
        while (o.outWidth / (sample * 2) >= width) sample *= 2
        return BitmapFactory.decodeByteArray(bytes, 0, bytes.size, BitmapFactory.Options().apply { inSampleSize = sample })
    }
}

/** Minimal `su` runner. */
object RootShell {
    fun run(script: String, timeoutSec: Long = 20): Pair<Boolean, String> = try {
        val p = ProcessBuilder("su").redirectErrorStream(true).start()
        p.outputStream.bufferedWriter().use { it.write(script + "\nexit \$?\n") }
        val out = StringBuilder()
        val reader = Thread { runCatching { out.append(p.inputStream.bufferedReader().readText()) } }
        reader.start()
        if (!p.waitFor(timeoutSec, TimeUnit.SECONDS)) {
            p.destroy()
            false to "su не ответил"
        } else {
            reader.join(1000)
            (p.exitValue() == 0) to out.toString().trim()
        }
    } catch (e: Exception) {
        false to (e.message ?: "su недоступен")
    }

    /** Raw bytes of a root-only file, or null. */
    fun readBytes(path: String, timeoutSec: Long = 15): ByteArray? = try {
        val p = ProcessBuilder("su", "-c", "cat '$path'").start()
        var bytes: ByteArray? = null
        val reader = Thread { bytes = runCatching { p.inputStream.readBytes() }.getOrNull() }
        reader.start()
        if (!p.waitFor(timeoutSec, TimeUnit.SECONDS)) {
            p.destroy()
            null
        } else {
            reader.join(5_000)
            bytes.takeIf { p.exitValue() == 0 }
        }
    } catch (e: Exception) {
        null
    }
}
