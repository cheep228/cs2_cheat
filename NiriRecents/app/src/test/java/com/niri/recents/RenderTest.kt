package com.niri.recents

import android.graphics.Bitmap
import android.graphics.Canvas
import android.graphics.LinearGradient
import android.graphics.Paint
import android.graphics.Shader
import android.view.View
import com.niri.recents.overview.DemoData
import com.niri.recents.overview.NiriOverviewView
import com.niri.recents.overview.OverviewStyle
import com.niri.recents.shared.NiriSettings
import org.junit.Assert.assertTrue
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner
import org.robolectric.RuntimeEnvironment
import org.robolectric.annotation.Config
import org.robolectric.annotation.GraphicsMode
import java.io.File

/** Renders the overview with mock windows to PNG files, to look at the result off-device. */
@RunWith(RobolectricTestRunner::class)
@Config(sdk = [34], qualifiers = "w411dp-h913dp-420dpi")
@GraphicsMode(GraphicsMode.Mode.NATIVE)
class RenderTest {

    private fun wallpaper(w: Int, h: Int): Bitmap {
        val b = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888)
        val p = Paint()
        p.shader = LinearGradient(0f, 0f, w.toFloat(), h.toFloat(), 0xFF3B2A6B.toInt(), 0xFFB8546A.toInt(), Shader.TileMode.CLAMP)
        Canvas(b).drawRect(0f, 0f, w.toFloat(), h.toFloat(), p)
        return b
    }

    private fun render(name: String, settings: NiriSettings) {
        val ctx = RuntimeEnvironment.getApplication()
        val dm = ctx.resources.displayMetrics
        val w = dm.widthPixels
        val h = dm.heightPixels
        val v = NiriOverviewView(ctx)
        v.respectSystemGestures = false
        v.style = OverviewStyle(settings, dm.density)
        v.wallpaper = wallpaper(540, 1200)
        val (spaces, active, focus) = DemoData.build(DemoData.syntheticApps(), w, h, dm.density)
        v.setData(spaces, active, focus)
        v.open(null)
        v.measure(View.MeasureSpec.makeMeasureSpec(w, View.MeasureSpec.EXACTLY), View.MeasureSpec.makeMeasureSpec(h, View.MeasureSpec.EXACTLY))
        v.layout(0, 0, w, h)
        v.snapAnimations()
        val out = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888)
        v.draw(Canvas(out))
        val dir = File("build/preview").apply { mkdirs() }
        File(dir, name).outputStream().use { out.compress(Bitmap.CompressFormat.PNG, 100, it) }

        // The focused window sits in the middle; the backdrop shows between workspaces.
        assertTrue(out.getPixel(w / 2, h / 2) != settings.backdrop)
        assertTrue((0 until h).any { out.getPixel(w / 2, it) == settings.backdrop })
    }

    @Test
    fun niriDefaults() = render("niri-default.png", NiriSettings())

    @Test
    fun customLook() = render(
        "niri-custom.png",
        NiriSettings(zoom = 0.42f, ringActive = 0xFFFFC87F.toInt(), labels = true, cornerDp = 24f, backdrop = 0xFF101010.toInt()),
    )
}
