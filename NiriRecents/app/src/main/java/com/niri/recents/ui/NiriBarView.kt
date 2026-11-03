package com.niri.recents.ui

import android.content.Context
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.graphics.RectF
import android.graphics.Typeface
import android.graphics.drawable.Drawable
import android.os.Bundle
import android.view.Gravity
import android.view.HapticFeedbackConstants
import android.view.MotionEvent
import android.view.View
import android.view.ViewConfiguration
import android.widget.PopupMenu
import com.niri.recents.TilerLink
import com.niri.recents.overview.Md3Palette
import com.niri.recents.shared.TilerProtocol
import kotlin.math.abs
import kotlin.math.hypot
import kotlin.math.roundToInt

/**
 * The tiling desktop's control bar: overview button, workspace dots, a strip map of the
 * workspace's columns (the part on screen framed), the focused column's width, and close.
 *
 * Tap a column in the map to focus it, drag the map sideways to scroll, swipe the bar up or
 * down to change workspace, long-press a column for more (move, full screen, close).
 */
class NiriBarView(context: Context) : View(context) {

    var bottomInset = 0

    private val d = resources.displayMetrics.density
    private var md3 = false
    private var palette: Md3Palette? = null
    private var accent = 0xFF7FC8FF.toInt()

    private var labels: Array<String> = emptyArray()
    private var packages: Array<String> = emptyArray()
    private var widths = FloatArray(0)
    private var focus = 0
    private var first = 0
    private var last = -1
    private var ws = 0
    private var wsCount = 0
    private val icons = HashMap<String, Drawable?>()

    private val pill = RectF()
    private val btnOverview = RectF()
    private val btnWidth = RectF()
    private val btnClose = RectF()
    private val map = RectF()
    private val segs = ArrayList<RectF>()
    private val tmp = RectF()

    private val fill = Paint(Paint.ANTI_ALIAS_FLAG)
    private val stroke = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        this.style = Paint.Style.STROKE
        strokeCap = Paint.Cap.ROUND
    }
    private val text = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        textAlign = Paint.Align.CENTER
        typeface = Typeface.create("sans-serif-medium", Typeface.NORMAL)
    }

    private val slop = ViewConfiguration.get(context).scaledTouchSlop.toFloat()
    private var downX = 0f
    private var downY = 0f
    private var downInMap = false
    private var dragging = false
    private var vertical = false
    private var longPressed = false
    private var pressedSeg = -1
    private var dragDx = 0f
    private val longPress = Runnable { onLongPress() }

    init {
        isClickable = true
        contentDescription = "Панель niri"
    }

    fun setLook(md3: Boolean, palette: Md3Palette?, accent: Int) {
        this.md3 = md3
        this.palette = palette
        this.accent = if (md3 && palette != null) palette.primary else accent
        invalidate()
    }

    fun setState(b: Bundle?) {
        labels = b?.getStringArray(TilerProtocol.S_LABELS) ?: emptyArray()
        packages = b?.getStringArray(TilerProtocol.S_PACKAGES) ?: emptyArray()
        widths = b?.getFloatArray(TilerProtocol.S_WIDTHS) ?: FloatArray(0)
        focus = b?.getInt(TilerProtocol.S_FOCUS) ?: 0
        first = b?.getInt(TilerProtocol.S_FIRST) ?: 0
        last = b?.getInt(TilerProtocol.S_LAST, -1) ?: -1
        ws = b?.getInt(TilerProtocol.S_WS) ?: 0
        wsCount = b?.getInt(TilerProtocol.S_WS_COUNT) ?: 0
        dragDx = 0f
        invalidate()
    }

    override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
        setMeasuredDimension(MeasureSpec.getSize(widthMeasureSpec), (TilerProtocol.BAR_DP * d).roundToInt() + bottomInset)
    }

    private val bg get() = if (md3) palette?.surfaceContainerHigh ?: 0xF0202020.toInt() else 0xEB1C1C1C.toInt()
    private val fg get() = if (md3) palette?.onSurface ?: Color.WHITE else 0xFFEDEDED.toInt()

    private fun layoutParts() {
        val m = 8f * d
        pill.set(m, m, width - m, m + 56f * d)
        val b = 48f * d
        btnOverview.set(pill.left + 4f * d, pill.top, pill.left + 4f * d + b, pill.bottom)
        btnClose.set(pill.right - 4f * d - b, pill.top, pill.right - 4f * d, pill.bottom)
        btnWidth.set(btnClose.left - b, pill.top, btnClose.left, pill.bottom)
        map.set(btnOverview.right + 20f * d, pill.top + 10f * d, btnWidth.left - 6f * d, pill.bottom - 10f * d)
    }

    private fun layoutSegments() {
        segs.clear()
        val n = widths.size
        if (n == 0 || map.width() <= 0f) return
        val gapS = 4f * d
        val sum = widths.sum().coerceAtLeast(0.01f)
        val unit = ((map.width() - gapS * (n - 1)) / sum).coerceIn(30f * d, 64f * d)
        val total = sum * unit + gapS * (n - 1)
        var x = if (total <= map.width()) {
            map.centerX() - total / 2f
        } else {
            // Too long to show whole: keep the part on screen in the middle.
            var acc = 0f
            var vs = 0f
            var ve = 0f
            val lastSafe = last.coerceIn(first, n - 1)
            for (i in 0 until n) {
                val w = widths[i] * unit
                if (i == first) vs = acc
                if (i == lastSafe) ve = acc + w
                acc += w + gapS
            }
            (map.centerX() - (vs + ve) / 2f).coerceIn(map.right - total, map.left)
        }
        x += dragDx
        for (i in 0 until n) {
            val w = widths[i] * unit
            segs += RectF(x, map.top, x + w, map.bottom)
            x += w + gapS
        }
    }

    override fun onDraw(c: Canvas) {
        layoutParts()
        layoutSegments()
        fill.color = bg
        c.drawRoundRect(pill, pill.height() / 2f, pill.height() / 2f, fill)
        drawOverviewIcon(c)
        drawWorkspaceDots(c)

        val save = c.save()
        c.clipRect(map.left - 4f * d, pill.top, map.right + 4f * d, pill.bottom)
        for ((i, r) in segs.withIndex()) {
            val visible = i in first..last
            fill.color = if (i == focus) accent else fg
            fill.alpha = when {
                i == focus -> 235
                visible -> 80
                else -> 38
            }
            c.drawRoundRect(r, 7f * d, 7f * d, fill)
            iconFor(packages.getOrNull(i))?.let { icon ->
                val s = (20f * d).roundToInt()
                if (r.width() >= s + 4f * d) {
                    val l = (r.centerX() - s / 2f).roundToInt()
                    val t = (r.centerY() - s / 2f).roundToInt()
                    icon.setBounds(l, t, l + s, t + s)
                    icon.alpha = if (visible || i == focus) 255 else 140
                    icon.draw(c)
                    icon.alpha = 255
                }
            }
        }
        if (last >= first && first in segs.indices) {
            val a = segs[first]
            val b = segs[last.coerceAtMost(segs.lastIndex)]
            tmp.set(a.left - 3f * d, map.top - 3f * d, b.right + 3f * d, map.bottom + 3f * d)
            stroke.color = accent
            stroke.alpha = 255
            stroke.strokeWidth = 2f * d
            c.drawRoundRect(tmp, 9f * d, 9f * d, stroke)
        }
        c.restoreToCount(save)
        if (segs.isEmpty()) {
            text.textSize = 13f * d
            text.color = fg
            text.alpha = 160
            c.drawText("Нет окон", map.centerX(), map.centerY() - (text.descent() + text.ascent()) / 2f, text)
        }

        // Width of the focused column, as niri's presets.
        text.textSize = 17f * d
        text.color = fg
        text.alpha = if (widths.isEmpty()) 90 else 255
        c.drawText(widthLabel(widths.getOrNull(focus)), btnWidth.centerX(), btnWidth.centerY() - (text.descent() + text.ascent()) / 2f, text)
        drawClose(c)
    }

    private fun widthLabel(w: Float?): String = when {
        w == null -> "½"
        w < 0.4f -> "⅓"
        w < 0.6f -> "½"
        w < 0.8f -> "⅔"
        else -> "1"
    }

    private fun drawOverviewIcon(c: Canvas) {
        val s = 7f * d
        val g = 3f * d
        val x0 = btnOverview.centerX() - s - g / 2f
        val y0 = btnOverview.centerY() - s - g / 2f
        fill.color = fg
        fill.alpha = 255
        for (k in 0 until 4) {
            val x = x0 + (k % 2) * (s + g)
            val y = y0 + (k / 2) * (s + g)
            tmp.set(x, y, x + s, y + s)
            c.drawRoundRect(tmp, 2f * d, 2f * d, fill)
        }
    }

    private fun drawWorkspaceDots(c: Canvas) {
        if (wsCount <= 1) return
        val x = btnOverview.right + 9f * d
        val shown = minOf(wsCount, 5)
        val step = 7f * d
        val top = pill.centerY() - step * (shown - 1) / 2f
        val firstShown = (ws - shown / 2).coerceIn(0, wsCount - shown)
        for (k in 0 until shown) {
            val idx = firstShown + k
            fill.color = if (idx == ws) accent else fg
            fill.alpha = if (idx == ws) 255 else 90
            c.drawCircle(x, top + k * step, if (idx == ws) 3f * d else 2.2f * d, fill)
        }
    }

    private fun drawClose(c: Canvas) {
        val a = 6f * d
        stroke.color = fg
        stroke.alpha = if (widths.isEmpty()) 90 else 255
        stroke.strokeWidth = 2f * d
        val cx = btnClose.centerX()
        val cy = btnClose.centerY()
        c.drawLine(cx - a, cy - a, cx + a, cy + a, stroke)
        c.drawLine(cx - a, cy + a, cx + a, cy - a, stroke)
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

    private fun segAt(x: Float, y: Float): Int {
        if (y < pill.top || y > pill.bottom) return -1
        return segs.indexOfFirst { x >= it.left - 2f * d && x <= it.right + 2f * d }
    }

    override fun performClick(): Boolean = super.performClick()

    override fun onTouchEvent(e: MotionEvent): Boolean {
        when (e.actionMasked) {
            MotionEvent.ACTION_DOWN -> {
                if (e.y < pill.top - 4f * d) return false
                downX = e.x
                downY = e.y
                downInMap = e.x in map.left..map.right
                dragging = false
                vertical = false
                longPressed = false
                pressedSeg = segAt(e.x, e.y)
                if (pressedSeg >= 0) postDelayed(longPress, ViewConfiguration.getLongPressTimeout().toLong())
                return true
            }
            MotionEvent.ACTION_MOVE -> {
                val dx = e.x - downX
                val dy = e.y - downY
                if (!dragging && !vertical && hypot(dx, dy) > slop) {
                    removeCallbacks(longPress)
                    if (abs(dx) > abs(dy) && downInMap) dragging = true else if (abs(dy) > abs(dx)) vertical = true
                }
                if (dragging) {
                    dragDx = dx
                    invalidate()
                }
            }
            MotionEvent.ACTION_UP -> {
                removeCallbacks(longPress)
                val dy = e.y - downY
                when {
                    longPressed -> Unit
                    dragging -> {
                        val n = widths.size
                        val step = if (segs.size >= 2) (segs.last().right - segs.first().left) / segs.size else map.width()
                        var steps = (-dragDx / step.coerceAtLeast(1f)).roundToInt()
                        if (steps == 0) steps = if (dragDx < 0f) 1 else -1
                        dragDx = 0f
                        if (n > 0) send(TilerProtocol.SCROLL, (first + steps).coerceIn(0, n - 1))
                        invalidate()
                    }
                    vertical -> when {
                        dy < -20f * d -> send(TilerProtocol.WORKSPACE, 1)
                        dy > 20f * d -> send(TilerProtocol.WORKSPACE, -1)
                    }
                    else -> tap(e.x, e.y)
                }
                dragging = false
                vertical = false
            }
            MotionEvent.ACTION_CANCEL -> {
                removeCallbacks(longPress)
                dragging = false
                vertical = false
                dragDx = 0f
                invalidate()
            }
        }
        return true
    }

    private fun tap(x: Float, y: Float) {
        performClick()
        when {
            btnOverview.contains(x, y) -> send(TilerProtocol.OVERVIEW)
            btnWidth.contains(x, y) -> send(TilerProtocol.WIDTH)
            btnClose.contains(x, y) -> send(TilerProtocol.CLOSE)
            else -> segAt(x, y).takeIf { it >= 0 }?.let { send(TilerProtocol.FOCUS, it) }
        }
    }

    private fun onLongPress() {
        val i = pressedSeg
        if (i < 0 || dragging || vertical) return
        longPressed = true
        performHapticFeedback(HapticFeedbackConstants.LONG_PRESS)
        send(TilerProtocol.FOCUS, i)
        val menu = PopupMenu(context, this, Gravity.TOP)
        menu.menu.add(0, 1, 0, "Сдвинуть влево")
        menu.menu.add(0, 2, 1, "Сдвинуть вправо")
        menu.menu.add(0, 3, 2, "На стол выше")
        menu.menu.add(0, 4, 3, "На стол ниже")
        menu.menu.add(0, 5, 4, "Открыть на весь экран")
        menu.menu.add(0, 6, 5, "Закрыть ${labels.getOrNull(i).orEmpty()}".trim())
        menu.setOnMenuItemClickListener { item ->
            when (item.itemId) {
                1 -> send(TilerProtocol.MOVE, -1)
                2 -> send(TilerProtocol.MOVE, 1)
                3 -> send(TilerProtocol.MOVE_WS, -1)
                4 -> send(TilerProtocol.MOVE_WS, 1)
                5 -> send(TilerProtocol.FULLSCREEN)
                6 -> send(TilerProtocol.CLOSE)
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
