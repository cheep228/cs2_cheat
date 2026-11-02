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
import com.niri.recents.overview.NiriOverviewView
import com.niri.recents.overview.OvColumn
import com.niri.recents.overview.OvWindow
import com.niri.recents.overview.OvWorkspace
import com.niri.recents.overview.OverviewStyle
import com.niri.recents.shared.NiriSettings
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
