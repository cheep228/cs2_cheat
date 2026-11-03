package com.niri.recents

import android.app.Activity
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.LinearGradient
import android.graphics.Paint
import android.graphics.RadialGradient
import android.graphics.RectF
import android.graphics.Shader
import android.os.Looper
import android.os.SystemClock
import android.view.MotionEvent
import android.view.View
import android.view.ViewGroup
import com.niri.recents.overview.DemoData
import com.niri.recents.overview.Md3Palette
import com.niri.recents.overview.NiriOverviewView
import com.niri.recents.overview.OvColumn
import com.niri.recents.overview.OvWindow
import com.niri.recents.overview.OvWorkspace
import com.niri.recents.overview.OverviewStyle
import com.niri.recents.shared.NiriSettings
import com.niri.recents.shared.TilerProtocol
import com.niri.recents.ui.NiriBarView
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.Robolectric
import org.robolectric.RobolectricTestRunner
import org.robolectric.RuntimeEnvironment
import org.robolectric.Shadows.shadowOf
import org.robolectric.annotation.Config
import org.robolectric.annotation.GraphicsMode
import java.io.File
import java.time.Duration
import kotlin.math.abs

/**
 * Renders the overview with mock windows to PNG files (build/preview), to look at the result
 * off-device. With -PpreviewThumbs=<dir> the windows show the images in that folder instead.
 */
@RunWith(RobolectricTestRunner::class)
@Config(sdk = [34], qualifiers = "w411dp-h913dp-420dpi")
@GraphicsMode(GraphicsMode.Mode.NATIVE)
class RenderTest {

    private val ctx get() = RuntimeEnvironment.getApplication()
    private val dm get() = ctx.resources.displayMetrics
    private val w get() = dm.widthPixels
    private val h get() = dm.heightPixels

    private fun wallpaper(): Bitmap {
        System.getProperty("niri.wallpaper")?.let { path -> BitmapFactory.decodeFile(path)?.let { return it } }
        val b = Bitmap.createBitmap(540, 1200, Bitmap.Config.ARGB_8888)
        val c = Canvas(b)
        val p = Paint(Paint.ANTI_ALIAS_FLAG)
        p.shader = LinearGradient(0f, 0f, 540f, 1200f, 0xFFE9C7A8.toInt(), 0xFF5A1F2E.toInt(), Shader.TileMode.CLAMP)
        c.drawRect(0f, 0f, 540f, 1200f, p)
        p.shader = RadialGradient(200f, 650f, 330f, 0x66FF7A3C, 0x00FF7A3C, Shader.TileMode.CLAMP)
        c.drawCircle(200f, 650f, 330f, p)
        return b
    }

    private fun thumbs(): List<Bitmap> {
        val dir = System.getProperty("niri.thumbs")?.let(::File) ?: return emptyList()
        return dir.listFiles().orEmpty().sortedBy { it.name }.mapNotNull { BitmapFactory.decodeFile(it.path) }
    }

    /** Mock apps, or the given screenshots when there are any. */
    private fun windows(count: Int): List<OvColumn> {
        val shots = thumbs()
        val apps = DemoData.syntheticApps()
        return (0 until count).map { i ->
            val app = apps[i % apps.size]
            val win = OvWindow(1000 + i, app.label, null)
            win.bitmap = shots.getOrNull(i % shots.size.coerceAtLeast(1))
                ?: DemoData.mockWindow(app, w / 3, h / 3, dm.density / 3f, i)
            OvColumn("k$i", intArrayOf(1000 + i), listOf(win), 1000L + i)
        }
    }

    private fun spaces(vararg sizes: Int): List<OvWorkspace> {
        val all = windows(sizes.sum())
        var k = 0
        return sizes.mapIndexed { i, n -> OvWorkspace(i + 1, all.subList(k, k + n).toMutableList().also { k += n }) }
    }

    private fun view(settings: NiriSettings): NiriOverviewView {
        val v = NiriOverviewView(ctx)
        v.respectSystemGestures = false
        v.fixedInsetTop = 36f * dm.density
        v.fixedInsetBottom = 24f * dm.density
        v.style = OverviewStyle(settings, dm.density)
        v.wallpaper = wallpaper()
        v.measure(View.MeasureSpec.makeMeasureSpec(w, View.MeasureSpec.EXACTLY), View.MeasureSpec.makeMeasureSpec(h, View.MeasureSpec.EXACTLY))
        v.layout(0, 0, w, h)
        return v
    }

    private fun shot(v: NiriOverviewView, name: String): Bitmap {
        val out = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888)
        v.draw(Canvas(out))
        val dir = File("build/preview").apply { mkdirs() }
        File(dir, name).outputStream().use { out.compress(Bitmap.CompressFormat.PNG, 100, it) }
        return out
    }

    private fun render(name: String, settings: NiriSettings, spaces: List<OvWorkspace>, active: Int, focus: String?): Bitmap {
        val v = view(settings)
        v.setData(spaces, active, focus)
        v.open(null)
        v.snapAnimations()
        return shot(v, name)
    }

    private val userLook = NiriSettings(
        ringActive = 0xFFD71921.toInt(),
        backdrop = 0xFF000000.toInt(),
        labels = true,
    )

    @Test
    fun niriDefaults() {
        val (spaces, active, focus) = DemoData.build(DemoData.syntheticApps(), w, h, dm.density)
        val out = render("niri-default.png", NiriSettings(), spaces, active, focus)
        // The backdrop (a little darker where workspace shadows fall) shows at the sides,
        // and the middle of the screen is a workspace.
        val side = out.getPixel(2, h / 2)
        assertTrue(abs(Color.red(side) - Color.red(NiriSettings().backdrop)) <= 12)
        assertTrue(out.getPixel(w / 2, h / 2) != NiriSettings().backdrop)
    }

    @Test
    fun labelsAndColours() {
        render("niri-labels.png", userLook, spaces(2, 3, 2), 1, "k3")
    }

    @Test
    fun oneLongWorkspace() {
        val out = render("niri-one-workspace.png", userLook, spaces(12), 0, "k11")
        // Short content is centred: backdrop above the first workspace and below the last.
        assertEquals(0xFF000000.toInt(), out.getPixel(w / 2, (60 * dm.density).toInt()))
        assertEquals(0xFF000000.toInt(), out.getPixel(w / 2, h - (40 * dm.density).toInt()))
    }

    @Test
    fun manyWorkspaces() {
        render("niri-many.png", NiriSettings(), spaces(3, 1, 4, 2), 2, "k5")
    }

    @Test
    fun materialDesign3() {
        val v = view(NiriSettings(md3 = true, labels = true))
        v.style = OverviewStyle(NiriSettings(md3 = true, labels = true), dm.density, Md3Palette.BASELINE)
        v.wallpaper = wallpaper()
        v.setData(spaces(3, 2), 1, "k4")
        v.open(null)
        v.snapAnimations()
        shot(v, "niri-md3.png")
        val plain = NiriSettings(md3 = true, labels = true, wallpaper = false, shadows = false)
        val noWallpaper = view(plain)
        noWallpaper.style = OverviewStyle(plain, dm.density, Md3Palette.BASELINE)
        noWallpaper.setData(spaces(3, 2), 1, "k4")
        noWallpaper.open(null)
        noWallpaper.snapAnimations()
        val out = shot(noWallpaper, "niri-md3-plain.png")
        assertEquals(Md3Palette.BASELINE.surface, out.getPixel(1, 1))
    }

    @Test
    fun pinchMakesWindowsSmaller() {
        val v = view(userLook)
        val activity = Robolectric.buildActivity(Activity::class.java).setup().get()
        activity.setContentView(v, ViewGroup.LayoutParams(w, h))
        shadowOf(Looper.getMainLooper()).idle()
        var saved = 0f
        v.host = object : NiriOverviewView.Host {
            override fun onLaunch(column: OvColumn) = Unit
            override fun onDismiss(column: OvColumn) = Unit
            override fun onHome() = Unit
            override fun onZoomChanged(zoom: Float) {
                saved = zoom
            }
        }
        v.setData(spaces(3, 3, 3, 3), 1, "k4")
        v.open(null)
        v.snapAnimations()
        val t0 = SystemClock.uptimeMillis()
        fun ev(action: Int, t: Long, spread: Float): MotionEvent {
            val props = arrayOf(MotionEvent.PointerProperties().apply { id = 0 }, MotionEvent.PointerProperties().apply { id = 1 })
            val coords = arrayOf(
                MotionEvent.PointerCoords().apply { x = w / 2f; y = h / 2f - spread },
                MotionEvent.PointerCoords().apply { x = w / 2f; y = h / 2f + spread },
            )
            return MotionEvent.obtain(t0, t, action, 2, props, coords, 0, 0, 1f, 1f, 0, 0, 0, 0)
        }
        v.dispatchTouchEvent(MotionEvent.obtain(t0, t0, MotionEvent.ACTION_DOWN, w / 2f, h / 2f - 300f, 0))
        v.dispatchTouchEvent(ev(MotionEvent.ACTION_POINTER_DOWN or (1 shl MotionEvent.ACTION_POINTER_INDEX_SHIFT), t0 + 10, 300f))
        v.dispatchTouchEvent(ev(MotionEvent.ACTION_MOVE, t0 + 60, 200f))
        v.dispatchTouchEvent(ev(MotionEvent.ACTION_MOVE, t0 + 120, 180f))
        v.dispatchTouchEvent(ev(MotionEvent.ACTION_POINTER_UP or (1 shl MotionEvent.ACTION_POINTER_INDEX_SHIFT), t0 + 150, 180f))
        v.dispatchTouchEvent(MotionEvent.obtain(t0, t0 + 160, MotionEvent.ACTION_UP, w / 2f, h / 2f - 180f, 0))
        v.snapAnimations()
        shot(v, "niri-pinched.png")
        // 0.34 * 180 / 300
        assertEquals(0.204f, saved, 0.01f)
    }

    /** What niri mode looks like: wallpaper, two live windows side by side, the control bar. */
    @Test
    fun tilingDesktopMock() {
        val d = dm.density
        val out = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888)
        val c = Canvas(out)
        val wp = wallpaper()
        c.drawBitmap(wp, null, android.graphics.Rect(0, 0, w, h), Paint(Paint.FILTER_BITMAP_FLAG))
        val bar = NiriBarView(ctx)
        bar.bottomInset = (24 * d).toInt()
        bar.setLook(false, null, userLook.ringActive)
        val state = android.os.Bundle().apply {
            putStringArray(TilerProtocol.S_LABELS, arrayOf("Telegram", "YouTube", "Chrome", "Карты"))
            putStringArray(TilerProtocol.S_PACKAGES, arrayOf("", "", "", ""))
            putFloatArray(TilerProtocol.S_WIDTHS, floatArrayOf(0.5f, 0.5f, 2f / 3f, 0.5f))
            putInt(TilerProtocol.S_FOCUS, 1)
            putInt(TilerProtocol.S_FIRST, 0)
            putInt(TilerProtocol.S_LAST, 1)
            putInt(TilerProtocol.S_WS, 1)
            putInt(TilerProtocol.S_WS_COUNT, 3)
        }
        bar.setState(state)
        bar.measure(View.MeasureSpec.makeMeasureSpec(w, View.MeasureSpec.EXACTLY), View.MeasureSpec.makeMeasureSpec(0, View.MeasureSpec.UNSPECIFIED))
        bar.layout(0, h - bar.measuredHeight, w, h)
        // Two half-width windows where the tiler would put them.
        val top = (36 * d + 6 * d).toInt()
        val bottom = h - (24 * d).toInt() - (TilerProtocol.BAR_DP * d).toInt()
        val gap = (8 * d).toInt()
        val edge = (6 * d).toInt()
        val colW = ((w - 2 * edge) - gap) / 2
        val apps = DemoData.syntheticApps()
        val ring = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            style = Paint.Style.STROKE
            strokeWidth = 3 * d
            color = userLook.ringActive
        }
        for (k in 0..1) {
            val l = edge + k * (colW + gap)
            val win = DemoData.mockWindow(apps[k], colW, bottom - top, d, k)
            val r = android.graphics.RectF(l.toFloat(), top.toFloat(), (l + colW).toFloat(), bottom.toFloat())
            val path = android.graphics.Path().apply { addRoundRect(r, 16 * d, 16 * d, android.graphics.Path.Direction.CW) }
            c.save()
            c.clipPath(path)
            c.drawBitmap(win, l.toFloat(), top.toFloat(), null)
            c.restore()
            if (k == 1) c.drawRoundRect(r, 16 * d, 16 * d, ring)
        }
        c.save()
        c.translate(0f, (h - bar.measuredHeight).toFloat())
        bar.draw(c)
        c.restore()
        val dir = File("build/preview").apply { mkdirs() }
        File(dir, "niri-tiling.png").outputStream().use { out.compress(Bitmap.CompressFormat.PNG, 100, it) }
    }

    @Test
    fun empty() {
        render("niri-empty.png", NiriSettings(), emptyList(), 0, null)
    }

    @Test
    fun openingZoom() {
        val v = view(userLook)
        v.setData(spaces(2, 3), 1, "k3")
        // The stock card of the app we came from, roughly where Launcher3 puts it.
        v.open(RectF(w * 0.18f, h * 0.2f, w * 0.82f, h * 0.2f + h * 0.64f))
        v.draw(Canvas(Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888)))
        shot(v, "niri-opening.png")
    }

    @Test
    fun dragToNewWorkspace() {
        val v = view(userLook)
        // Attached to a window, so the long-press timer runs.
        val activity = Robolectric.buildActivity(Activity::class.java).setup().get()
        activity.setContentView(v, ViewGroup.LayoutParams(w, h))
        shadowOf(Looper.getMainLooper()).idle()
        assertEquals(w, v.width)
        v.setData(spaces(2, 3), 1, "k3")
        v.open(null)
        v.snapAnimations()
        shot(v, "niri-before-drag.png")
        // Long-press the first window of the second workspace and carry it between the two.
        val t0 = SystemClock.uptimeMillis()
        val x = w * 0.2f
        val y = h * 0.62f
        v.dispatchTouchEvent(MotionEvent.obtain(t0, t0, MotionEvent.ACTION_DOWN, x, y, 0))
        shadowOf(Looper.getMainLooper()).idleFor(Duration.ofMillis(700))
        v.dispatchTouchEvent(MotionEvent.obtain(t0, t0 + 750, MotionEvent.ACTION_MOVE, x + 40f, h * 0.47f, 0))
        v.snapAnimations()
        shot(v, "niri-drag.png")
        v.dispatchTouchEvent(MotionEvent.obtain(t0, t0 + 800, MotionEvent.ACTION_UP, x + 40f, h * 0.47f, 0))
        v.snapAnimations()
        shot(v, "niri-after-drag.png")
        assertEquals(3, v.workspacesSnapshot().size)
    }
}
