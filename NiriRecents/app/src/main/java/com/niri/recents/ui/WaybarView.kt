package com.niri.recents.ui

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.graphics.Canvas
import android.graphics.LinearGradient
import android.graphics.Paint
import android.graphics.Path
import android.graphics.RectF
import android.graphics.Shader
import android.graphics.Typeface
import android.graphics.drawable.Drawable
import android.os.BatteryManager
import android.os.Bundle
import android.text.TextPaint
import android.text.TextUtils
import android.view.MotionEvent
import android.view.View
import android.view.ViewConfiguration
import android.widget.PopupMenu
import com.niri.recents.TilerLink
import com.niri.recents.overview.OverviewStyle
import com.niri.recents.shared.TilerProtocol
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale
import kotlin.math.abs
import kotlin.math.roundToInt

/**
 * The Hyprland desktop's top bar, in the style of a floating-modules Waybar:
 * logo (back to the overview) and workspace buttons on the left, the focused window in the
 * middle (tap for window actions, ✕ to close), clock and battery on the right.
 * Swiping the bar sideways switches workspace, like Hyprland's workspace swipe.
 */
class WaybarView(context: Context) : View(context) {

    var topInset = 0

    private val d = resources.displayMetrics.density

    private var labels: Array<String> = emptyArray()
    private var packages: Array<String> = emptyArray()
    private var focus = 0
    private var ws = 0
    private var wsCount = 0
    private var clock = ""
    private var battery = -1
    private var charging = false
    private val icons = HashMap<String, Drawable?>()

    private val modBg = 0xE61E1E2E.toInt()
    private val ink = 0xFFCDD6F4.toInt()
    private val ink2 = 0x99CDD6F4.toInt()
    private val dark = 0xFF11111B.toInt()

    private val left = RectF()
    private val center = RectF()
    private val right = RectF()
    private val logo = RectF()
    private val wsButtons = ArrayList<RectF>()
    private val close = RectF()
    private val tmp = RectF()
    private val path = Path()

    private val fill = Paint(Paint.ANTI_ALIAS_FLAG)
    private val stroke = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        this.style = Paint.Style.STROKE
        strokeCap = Paint.Cap.ROUND
    }
    private val text = TextPaint(Paint.ANTI_ALIAS_FLAG).apply {
        typeface = Typeface.create("sans-serif-medium", Typeface.NORMAL)
    }

    private val slop = ViewConfiguration.get(context).scaledTouchSlop.toFloat()
    private var downX = 0f
    private var downY = 0f
    private var swiping = false

    private val tick = object : Runnable {
        override fun run() {
            clock = SimpleDateFormat("HH:mm", Locale.getDefault()).format(Date())
            invalidate()
            postDelayed(this, 15_000)
        }
    }

    private val batteryReceiver = object : BroadcastReceiver() {
        override fun onReceive(c: Context, i: Intent) = readBattery(i)
    }

    init {
        isClickable = true
        contentDescription = "Waybar"
    }

    fun setState(b: Bundle?) {
        labels = b?.getStringArray(TilerProtocol.S_LABELS) ?: emptyArray()
        packages = b?.getStringArray(TilerProtocol.S_PACKAGES) ?: emptyArray()
        focus = b?.getInt(TilerProtocol.S_FOCUS) ?: 0
        ws = b?.getInt(TilerProtocol.S_WS) ?: 0
        wsCount = b?.getInt(TilerProtocol.S_WS_COUNT) ?: 0
        invalidate()
    }

    override fun onAttachedToWindow() {
        super.onAttachedToWindow()
        tick.run()
        try {
            context.registerReceiver(batteryReceiver, IntentFilter(Intent.ACTION_BATTERY_CHANGED), Context.RECEIVER_NOT_EXPORTED)
                ?.let { readBattery(it) }
        } catch (t: Throwable) {
            // No battery info.
        }
    }

    override fun onDetachedFromWindow() {
        removeCallbacks(tick)
        try {
            context.unregisterReceiver(batteryReceiver)
        } catch (t: Throwable) {
            // Not registered.
        }
        super.onDetachedFromWindow()
    }

    private fun readBattery(i: Intent) {
        val level = i.getIntExtra(BatteryManager.EXTRA_LEVEL, -1)
        val scale = i.getIntExtra(BatteryManager.EXTRA_SCALE, 100).coerceAtLeast(1)
        battery = if (level < 0) -1 else level * 100 / scale
        val status = i.getIntExtra(BatteryManager.EXTRA_STATUS, -1)
        charging = status == BatteryManager.BATTERY_STATUS_CHARGING || status == BatteryManager.BATTERY_STATUS_FULL
        invalidate()
    }

    override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
        setMeasuredDimension(MeasureSpec.getSize(widthMeasureSpec), topInset + (TilerProtocol.WAYBAR_DP * d).roundToInt())
    }

    private fun layoutModules() {
        if (clock.isEmpty()) clock = SimpleDateFormat("HH:mm", Locale.getDefault()).format(Date())
        val top = topInset + 6f * d
        val bottom = top + 34f * d
        val m = 8f * d
        // Left: logo + workspace numbers.
        logo.set(m, top, m + 34f * d, bottom)
        wsButtons.clear()
        var x = logo.right
        val shown = wsCount.coerceIn(1, 9)
        for (i in 0 until shown) {
            wsButtons += RectF(x + 2f * d, top + 5f * d, x + 26f * d, bottom - 5f * d)
            x += 26f * d
        }
        left.set(m, top, x + 6f * d, bottom)
        // Right: clock and battery.
        text.textSize = 14f * d
        val clockW = text.measureText(clock)
        val batW = if (battery >= 0) 22f * d + text.measureText("$battery%") + 6f * d else 0f
        right.set(width - m - (clockW + batW + 24f * d), top, width - m, bottom)
        // Middle: the focused window.
        center.set(left.right + 6f * d, top, right.left - 6f * d, bottom)
        close.set(center.right - 30f * d, top, center.right, bottom)
    }

    override fun onDraw(c: Canvas) {
        layoutModules()
        val radius = 10f * d
        fill.shader = null
        fill.color = modBg
        c.drawRoundRect(left, radius, radius, fill)
        c.drawRoundRect(right, radius, radius, fill)
        if (center.width() > 60f * d) c.drawRoundRect(center, radius, radius, fill)

        drawLogo(c)
        text.textSize = 13f * d
        text.textAlign = Paint.Align.CENTER
        for ((i, r) in wsButtons.withIndex()) {
            if (i == ws) {
                fill.shader = LinearGradient(r.left, r.top, r.right, r.bottom, OverviewStyle.HYPR_ACTIVE_A, OverviewStyle.HYPR_ACTIVE_B, Shader.TileMode.CLAMP)
                c.drawRoundRect(r, 7f * d, 7f * d, fill)
                fill.shader = null
                text.color = dark
            } else {
                text.color = ink2
            }
            c.drawText("${i + 1}", r.centerX(), r.centerY() - (text.descent() + text.ascent()) / 2f, text)
        }

        if (center.width() > 60f * d) drawTitle(c)

        // Clock and battery.
        text.textSize = 14f * d
        text.textAlign = Paint.Align.LEFT
        text.color = ink
        var x = right.left + 12f * d
        c.drawText(clock, x, right.centerY() - (text.descent() + text.ascent()) / 2f, text)
        x += text.measureText(clock) + 10f * d
        if (battery >= 0) {
            drawBattery(c, x, right.centerY())
            x += 22f * d
            c.drawText("$battery%", x, right.centerY() - (text.descent() + text.ascent()) / 2f, text)
        }
    }

    /** A droplet in Hyprland's colours. */
    private fun drawLogo(c: Canvas) {
        val cx = logo.centerX()
        val cy = logo.centerY() + 2f * d
        val r = 6.5f * d
        val top = cy - 2.1f * r
        path.rewind()
        path.moveTo(cx, top)
        path.cubicTo(cx + r * 0.55f, top + r * 0.9f, cx + r, cy - r * 0.25f, cx + r, cy + r * 0.2f)
        tmp.set(cx - r, cy - r * 0.8f, cx + r, cy + r * 1.2f)
        path.arcTo(tmp, 0f, 180f)
        path.cubicTo(cx - r, cy - r * 0.25f, cx - r * 0.55f, top + r * 0.9f, cx, top)
        path.close()
        fill.shader = LinearGradient(cx - r, top, cx + r, cy + r, OverviewStyle.HYPR_ACTIVE_A, OverviewStyle.HYPR_ACTIVE_B, Shader.TileMode.CLAMP)
        c.drawPath(path, fill)
        fill.shader = null
    }

    private fun drawTitle(c: Canvas) {
        val label = labels.getOrNull(focus)
        val icon = iconFor(packages.getOrNull(focus))
        var x = center.left + 12f * d
        if (icon != null) {
            val s = (18f * d).roundToInt()
            val t = (center.centerY() - s / 2f).roundToInt()
            icon.setBounds(x.roundToInt(), t, x.roundToInt() + s, t + s)
            icon.draw(c)
            x += s + 8f * d
        }
        text.textSize = 13.5f * d
        text.textAlign = Paint.Align.LEFT
        text.color = ink
        val room = close.left - x - 4f * d
        val shown = TextUtils.ellipsize(label ?: "Нет окон", text, room.coerceAtLeast(0f), TextUtils.TruncateAt.END).toString()
        c.drawText(shown, x, center.centerY() - (text.descent() + text.ascent()) / 2f, text)
        if (label != null) {
            val a = 5f * d
            stroke.strokeWidth = 1.8f * d
            stroke.color = ink2
            c.drawLine(close.centerX() - a, close.centerY() - a, close.centerX() + a, close.centerY() + a, stroke)
            c.drawLine(close.centerX() - a, close.centerY() + a, close.centerX() + a, close.centerY() - a, stroke)
        }
    }

    private fun drawBattery(c: Canvas, x: Float, cy: Float) {
        val w = 17f * d
        val h = 9f * d
        tmp.set(x, cy - h / 2f, x + w, cy + h / 2f)
        stroke.strokeWidth = 1.4f * d
        stroke.color = ink
        c.drawRoundRect(tmp, 2.5f * d, 2.5f * d, stroke)
        fill.color = if (charging) 0xFFA6E3A1.toInt() else if (battery <= 15) 0xFFF38BA8.toInt() else ink
        val inner = (w - 4f * d) * (battery.coerceIn(0, 100) / 100f)
        tmp.set(x + 2f * d, cy - h / 2f + 2f * d, x + 2f * d + inner, cy + h / 2f - 2f * d)
        c.drawRoundRect(tmp, 1f * d, 1f * d, fill)
        tmp.set(x + w + 0.5f * d, cy - 2f * d, x + w + 2.2f * d, cy + 2f * d)
        fill.color = ink
        c.drawRoundRect(tmp, 1f * d, 1f * d, fill)
    }

    private fun iconFor(pkg: String?): Drawable? {
        if (pkg.isNullOrEmpty()) return null
        return icons.getOrPut(pkg) {
            try {
                context.packageManager.getApplicationIcon(pkg).mutate()
            } catch (t: Throwable) {
                null
            }
        }
    }

    override fun performClick(): Boolean = super.performClick()

    override fun onTouchEvent(e: MotionEvent): Boolean {
        when (e.actionMasked) {
            MotionEvent.ACTION_DOWN -> {
                if (e.y < topInset) return false
                downX = e.x
                downY = e.y
                swiping = false
                return true
            }
            MotionEvent.ACTION_MOVE -> {
                if (!swiping && abs(e.x - downX) > slop * 2 && abs(e.x - downX) > abs(e.y - downY)) swiping = true
            }
            MotionEvent.ACTION_UP -> {
                val dx = e.x - downX
                if (swiping) {
                    if (abs(dx) > 40f * d) send(TilerProtocol.WORKSPACE, if (dx < 0f) 1 else -1)
                } else {
                    tap(e.x, e.y)
                }
                swiping = false
            }
            MotionEvent.ACTION_CANCEL -> swiping = false
        }
        return true
    }

    private fun tap(x: Float, y: Float) {
        performClick()
        when {
            logo.contains(x, y) -> send(TilerProtocol.OVERVIEW)
            close.contains(x, y) && labels.isNotEmpty() -> send(TilerProtocol.CLOSE)
            center.contains(x, y) && labels.isNotEmpty() -> windowMenu()
            else -> wsButtons.indexOfFirst { it.left - 2f * d <= x && x <= it.right + 2f * d && left.contains(x, y) }
                .takeIf { it >= 0 }?.let { send(TilerProtocol.WORKSPACE_TO, it) }
        }
    }

    private fun windowMenu() {
        val menu = PopupMenu(context, this)
        menu.menu.add(0, 1, 0, "Разделить по-другому (togglesplit)")
        menu.menu.add(0, 2, 1, "Поменять с предыдущим")
        menu.menu.add(0, 3, 2, "Поменять со следующим")
        if (ws > 0) menu.menu.add(0, 4, 3, "На стол $ws")
        menu.menu.add(0, 5, 4, "На стол ${ws + 2}")
        menu.menu.add(0, 6, 5, "Открыть на весь экран")
        menu.menu.add(0, 7, 6, "Закрыть")
        menu.setOnMenuItemClickListener { item ->
            when (item.itemId) {
                1 -> send(TilerProtocol.TOGGLE_SPLIT)
                2 -> send(TilerProtocol.MOVE, -1)
                3 -> send(TilerProtocol.MOVE, 1)
                4 -> send(TilerProtocol.MOVE_WS, -1)
                5 -> send(TilerProtocol.MOVE_WS, 1)
                6 -> send(TilerProtocol.FULLSCREEN)
                7 -> send(TilerProtocol.CLOSE)
            }
            true
        }
        menu.show()
    }

    private fun send(code: Int, vararg args: Int) {
        if (!TilerLink.send(code, *args)) {
            android.widget.Toast.makeText(context, "Лаунчер не отвечает: откройте «Недавние» и нажмите на окно", android.widget.Toast.LENGTH_SHORT).show()
        }
    }
}
