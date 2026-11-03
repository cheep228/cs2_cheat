package com.niri.recents.ui

import android.content.Context
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.LinearGradient
import android.graphics.Paint
import android.graphics.RectF
import android.graphics.Shader
import android.os.Bundle
import android.view.View
import com.niri.recents.overview.OverviewStyle
import com.niri.recents.shared.TilerProtocol

/**
 * Drawn on the desktop, behind the live windows: the frame of each window shows in the gaps.
 * niri mode rings the focused window (focus-ring); Hyprland mode borders every window, the
 * focused one with the gradient, and adds a soft shadow under each.
 */
class WindowBordersView(context: Context) : View(context) {

    private val d = resources.displayMetrics.density
    private var rects = IntArray(0)
    private var focus = -1
    private var hypr = false
    private var ringColor = 0xFF7FC8FF.toInt()

    private val stroke = Paint(Paint.ANTI_ALIAS_FLAG).apply { this.style = Paint.Style.STROKE }
    private val shadow = Paint(Paint.ANTI_ALIAS_FLAG)
    private val r = RectF()
    private val loc = IntArray(2)

    fun setRingColor(color: Int) {
        ringColor = color
        invalidate()
    }

    fun setState(b: Bundle?) {
        rects = b?.getIntArray(TilerProtocol.S_RECTS) ?: IntArray(0)
        focus = b?.getInt(TilerProtocol.S_RECT_FOCUS, -1) ?: -1
        hypr = b?.getString(TilerProtocol.S_MODE) == TilerProtocol.MODE_HYPR
        invalidate()
    }

    override fun onDraw(c: Canvas) {
        getLocationOnScreen(loc)
        val n = rects.size / 4
        for (i in 0 until n) {
            r.set(
                (rects[4 * i] - loc[0]).toFloat(),
                (rects[4 * i + 1] - loc[1]).toFloat(),
                (rects[4 * i + 2] - loc[0]).toFloat(),
                (rects[4 * i + 3] - loc[1]).toFloat(),
            )
            val focused = i == focus
            if (hypr) {
                // decoration:shadow, then the border (border_size 2, rounding 10).
                val radius = 10f * d
                // The shape casting it sits a little inside the window, which hides it.
                shadow.color = 0xFF1A1A1A.toInt()
                shadow.setShadowLayer(9f * d, 0f, 2f * d, 0xCC1A1A1A.toInt())
                val inset = 3f * d
                c.drawRoundRect(r.left + inset, r.top + inset, r.right - inset, r.bottom - inset, 14f * d, 14f * d, shadow)
                val bw = 2.5f * d
                stroke.strokeWidth = bw
                if (focused) {
                    stroke.color = Color.WHITE
                    stroke.shader = LinearGradient(
                        r.left, r.top, r.right, r.bottom,
                        OverviewStyle.HYPR_ACTIVE_A, OverviewStyle.HYPR_ACTIVE_B, Shader.TileMode.CLAMP,
                    )
                } else {
                    stroke.shader = null
                    stroke.color = OverviewStyle.HYPR_INACTIVE
                }
                drawFrame(c, radius, bw)
                stroke.shader = null
            } else if (focused) {
                val bw = 3f * d
                stroke.strokeWidth = bw
                stroke.shader = null
                stroke.color = ringColor
                drawFrame(c, 14f * d, bw)
            }
        }
    }

    /** A frame just outside the window, so it shows in the gap around it. */
    private fun drawFrame(c: Canvas, radius: Float, bw: Float) {
        val o = bw / 2f
        r.inset(-o, -o)
        c.drawRoundRect(r, radius + o, radius + o, stroke)
    }
}
