package com.niri.recents.ui

import android.app.Activity
import android.graphics.Color
import android.graphics.Rect
import android.os.Build
import android.os.Bundle
import android.view.Gravity
import android.view.View
import android.view.ViewGroup
import android.view.WindowInsets
import android.widget.FrameLayout
import android.widget.TextView
import android.window.OnBackInvokedDispatcher
import com.niri.recents.SettingsStore
import com.niri.recents.TilerLink
import com.niri.recents.overview.Md3Palette
import com.niri.recents.shared.TilerProtocol
import kotlin.math.roundToInt

/**
 * niri's desktop: the wallpaper behind the tiled app windows, with a control bar at the bottom.
 * The launcher lays the windows out over it; this activity only shows the bar and passes the
 * user's commands back to the launcher.
 */
class NiriDesktopActivity : Activity() {

    private lateinit var root: FrameLayout
    private lateinit var bar: NiriBarView
    private lateinit var hint: TextView
    private val onState: () -> Unit = { render() }
    private var lastHello: Rect? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        window.setDecorFitsSystemWindows(false)
        val d = resources.displayMetrics.density
        root = FrameLayout(this)
        hint = TextView(this).apply {
            setTextColor(Color.argb(220, 255, 255, 255))
            textSize = 15f
            gravity = Gravity.CENTER
            setShadowLayer(6f * d, 0f, 1f * d, Color.argb(160, 0, 0, 0))
            val p = (32 * d).roundToInt()
            setPadding(p, p, p, p)
        }
        bar = NiriBarView(this)
        root.addView(hint, FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT, Gravity.CENTER))
        root.addView(bar, FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT, Gravity.BOTTOM))
        setContentView(root)
        root.setOnApplyWindowInsetsListener { _, insets ->
            bar.bottomInset = insets.getInsets(WindowInsets.Type.navigationBars()).bottom
            bar.requestLayout()
            root.post { sendHello() }
            insets
        }
        root.addOnLayoutChangeListener { _, _, _, _, _, _, _, _, _ -> sendHello() }
        if (Build.VERSION.SDK_INT >= 33) {
            onBackInvokedDispatcher.registerOnBackInvokedCallback(OnBackInvokedDispatcher.PRIORITY_DEFAULT) {
                TilerLink.send(TilerProtocol.OVERVIEW)
            }
        }
    }

    @Deprecated("Back goes to the overview")
    override fun onBackPressed() {
        TilerLink.send(TilerProtocol.OVERVIEW)
    }

    override fun onStart() {
        super.onStart()
        val s = SettingsStore.load(this)
        bar.setLook(s.md3, if (s.md3) Md3Palette.system(this) else null, s.ringActive)
        TilerLink.listen(onState)
        shownSent = TilerLink.send(TilerProtocol.SHOWN)
        lastHello = null
        sendHello()
        render()
    }

    /** False until the launcher got SHOWN (on a cold start its channel arrives a bit later). */
    private var shownSent = false

    override fun onStop() {
        TilerLink.send(TilerProtocol.HIDDEN)
        TilerLink.unlisten(onState)
        super.onStop()
    }

    /** Tells the launcher where windows may go: everything but the status bar and our bar. */
    private fun sendHello() {
        val v = window.decorView
        if (v.width == 0 || v.height == 0) return
        val insets = v.rootWindowInsets ?: return
        val i = insets.getInsets(WindowInsets.Type.systemBars() or WindowInsets.Type.displayCutout())
        val d = resources.displayMetrics.density
        val loc = IntArray(2)
        v.getLocationOnScreen(loc)
        val edge = (6f * d).roundToInt()
        val r = Rect(
            loc[0] + edge,
            loc[1] + i.top + edge,
            loc[0] + v.width - edge,
            loc[1] + v.height - i.bottom - (TilerProtocol.BAR_DP * d).roundToInt(),
        )
        if (r == lastHello) return
        if (TilerLink.send(TilerProtocol.HELLO, r.left, r.top, r.right, r.bottom)) lastHello = r
    }

    private fun render() {
        // The first state also brings the launcher's channel: catch up on what couldn't be sent.
        if (!shownSent && TilerLink.binder != null) shownSent = TilerLink.send(TilerProtocol.SHOWN)
        sendHello()
        val s = TilerLink.state
        bar.setState(s)
        val text = when {
            TilerLink.binder == null ->
                "Режим niri ещё не подключён.\nОткройте «Недавние» и нажмите на окно."
            s?.getString(TilerProtocol.S_MESSAGE) != null -> s.getString(TilerProtocol.S_MESSAGE)
            else -> null
        }
        hint.text = text ?: ""
        hint.visibility = if (text == null) View.GONE else View.VISIBLE
    }
}
