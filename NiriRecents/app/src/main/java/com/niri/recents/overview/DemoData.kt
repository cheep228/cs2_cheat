package com.niri.recents.overview

import android.content.Context
import android.content.Intent
import android.graphics.Bitmap
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.graphics.RectF
import android.graphics.drawable.Drawable
import kotlin.math.max

/** Fake windows for the preview in the settings app (and for screenshot tests). */
object DemoData {

    class DemoApp(val label: String, val color: Int, val icon: Drawable?)

    /** Two workspaces of mock windows; returns spaces, active index and focused key. */
    fun build(apps: List<DemoApp>, screenW: Int, screenH: Int, density: Float): Triple<List<OvWorkspace>, Int, String?> {
        if (apps.isEmpty()) return Triple(emptyList(), 0, null)
        val bw = max(120, screenW / 3)
        val bh = max(240, screenH / 3)
        var id = 100
        var time = 1_000L
        fun column(app: DemoApp, variant: Int): OvColumn {
            val win = OvWindow(++id, app.label, null)
            win.icon = app.icon
            win.bitmap = mockWindow(app, bw, bh, density / 3f, variant)
            return OvColumn(id.toString(), intArrayOf(id), listOf(win), time++)
        }
        // Three workspaces with the middle one active, so both neighbours peek in.
        val top = apps.take(2).mapIndexed { i, a -> column(a, i) }
        val middle = apps.drop(2).take(3).mapIndexed { i, a -> column(a, i + 2) }
        val bottom = apps.drop(5).take(2).mapIndexed { i, a -> column(a, i + 5) }
        val spaces = listOf(top, middle, bottom).filter { it.isNotEmpty() }
            .mapIndexed { i, cols -> OvWorkspace(i + 1, cols.toMutableList()) }
        val active = if (spaces.size >= 2) 1 else 0
        val focusCols = spaces[active].columns
        val focus = focusCols.getOrNull(1) ?: focusCols.last()
        return Triple(spaces, active, focus.key)
    }

    fun installedApps(context: Context, count: Int = 7): List<DemoApp> {
        val pm = context.packageManager
        val intent = Intent(Intent.ACTION_MAIN).addCategory(Intent.CATEGORY_LAUNCHER)
        return try {
            pm.queryIntentActivities(intent, 0)
                .filter { it.activityInfo.packageName != context.packageName }
                .distinctBy { it.activityInfo.packageName }
                .sortedBy { it.loadLabel(pm).toString().lowercase() }
                .take(count)
                .map { ri ->
                    val icon = ri.loadIcon(pm)
                    DemoApp(ri.loadLabel(pm).toString(), accentOf(icon), icon)
                }
        } catch (t: Throwable) {
            emptyList()
        }.ifEmpty { syntheticApps() }
    }

    fun syntheticApps(): List<DemoApp> = listOf(
        DemoApp("Telegram", 0xFF2AABEE.toInt(), null),
        DemoApp("YouTube", 0xFFE53935.toInt(), null),
        DemoApp("Chrome", 0xFF34A853.toInt(), null),
        DemoApp("Spotify", 0xFF1DB954.toInt(), null),
        DemoApp("Карты", 0xFFFBBC04.toInt(), null),
        DemoApp("Камера", 0xFF8E24AA.toInt(), null),
        DemoApp("Настройки", 0xFF607D8B.toInt(), null),
    )

    /** Average colour of an icon, to tint its mock window. */
    private fun accentOf(icon: Drawable): Int = try {
        val b = Bitmap.createBitmap(24, 24, Bitmap.Config.ARGB_8888)
        icon.setBounds(0, 0, 24, 24)
        icon.draw(Canvas(b))
        var r = 0L
        var g = 0L
        var bl = 0L
        var n = 0L
        for (x in 0 until 24) for (y in 0 until 24) {
            val p = b.getPixel(x, y)
            if (Color.alpha(p) < 200) continue
            val sat = max(Color.red(p), max(Color.green(p), Color.blue(p))) - minOf(Color.red(p), Color.green(p), Color.blue(p))
            if (sat < 40) continue
            r += Color.red(p)
            g += Color.green(p)
            bl += Color.blue(p)
            n++
        }
        if (n == 0L) 0xFF5C6BC0.toInt() else Color.rgb((r / n).toInt(), (g / n).toInt(), (bl / n).toInt())
    } catch (t: Throwable) {
        0xFF5C6BC0.toInt()
    }

    /** A plausible app screen: status bar, coloured toolbar, a few cards, a bottom bar. */
    fun mockWindow(app: DemoApp, w: Int, h: Int, density: Float, variant: Int): Bitmap {
        val bmp = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888)
        val c = Canvas(bmp)
        val dark = variant % 2 == 0
        val bg = if (dark) 0xFF121212.toInt() else 0xFFF3F3F3.toInt()
        val card = if (dark) 0xFF1F1F1F.toInt() else 0xFFFFFFFF.toInt()
        val line = if (dark) 0xFF3A3A3A.toInt() else 0xFFDADADA.toInt()
        val p = Paint(Paint.ANTI_ALIAS_FLAG)
        c.drawColor(bg)
        val dp = max(0.5f, density)
        val bar = 56f * dp
        val status = 24f * dp
        p.color = app.color
        c.drawRect(0f, 0f, w.toFloat(), status + bar, p)
        app.icon?.let {
            val s = (28f * dp).toInt()
            val l = (14f * dp).toInt()
            val t = (status + (bar - s) / 2f).toInt()
            it.setBounds(l, t, l + s, t + s)
            it.draw(c)
        }
        p.color = Color.WHITE
        p.textSize = 18f * dp
        c.drawText(app.label, 52f * dp, status + bar / 2f + 6f * dp, p)
        var y = status + bar + 12f * dp
        val r = RectF()
        var i = 0
        while (y < h - 80f * dp) {
            val tall = if ((i + variant) % 3 == 0) 120f * dp else 64f * dp
            r.set(12f * dp, y, w - 12f * dp, y + tall)
            p.color = card
            c.drawRoundRect(r, 12f * dp, 12f * dp, p)
            if (tall > 100f * dp) {
                p.color = (app.color and 0x00FFFFFF) or 0x55000000
                c.drawRoundRect(RectF(r.left + 8f * dp, r.top + 8f * dp, r.right - 8f * dp, r.bottom - 34f * dp), 8f * dp, 8f * dp, p)
            } else {
                p.color = app.color
                c.drawCircle(r.left + 30f * dp, r.centerY(), 16f * dp, p)
            }
            p.color = line
            c.drawRoundRect(RectF(r.left + 58f * dp, r.bottom - 26f * dp, r.right - 60f * dp, r.bottom - 18f * dp), 4f * dp, 4f * dp, p)
            c.drawRoundRect(RectF(r.left + 58f * dp, r.bottom - 14f * dp, r.right - 110f * dp, r.bottom - 8f * dp), 3f * dp, 3f * dp, p)
            y += tall + 10f * dp
            i++
        }
        p.color = card
        c.drawRect(0f, h - 56f * dp, w.toFloat(), h.toFloat(), p)
        p.color = app.color
        for (k in 0 until 4) c.drawCircle(w * (k + 0.5f) / 4f, h - 28f * dp, 7f * dp, p)
        return bmp
    }
}
