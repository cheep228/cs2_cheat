package com.niri.recents.overview

import android.content.Context
import android.graphics.Bitmap
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.graphics.Path
import android.graphics.Rect
import android.graphics.RectF
import android.graphics.drawable.Drawable
import android.text.TextPaint
import android.text.TextUtils
import android.view.HapticFeedbackConstants
import android.view.MotionEvent
import android.view.VelocityTracker
import android.view.View
import android.view.ViewConfiguration
import android.view.WindowInsets
import com.niri.recents.shared.NiriSettings
import kotlin.math.abs
import kotlin.math.floor
import kotlin.math.hypot
import kotlin.math.max
import kotlin.math.min
import kotlin.math.roundToInt

/** [NiriSettings] resolved to pixels. */
class OverviewStyle(s: NiriSettings, val density: Float) {
    val zoom = s.zoom.coerceIn(0.2f, 0.85f)
    val gap = s.gapsDp.coerceAtLeast(0f) * density
    val wsGap = s.workspaceGapDp.coerceAtLeast(0f) * density
    val corner = s.cornerDp.coerceAtLeast(0f) * density
    val ring = s.ringWidthDp.coerceAtLeast(0f) * density
    val ringActive = s.ringActive
    val ringInactive = s.ringInactive
    val backdrop = s.backdrop or 0xFF000000.toInt()
    val workspaceColor = s.workspaceColor or 0xFF000000.toInt()
    val useWallpaper = s.wallpaper
    val shadows = s.shadows
    val labels = s.labels
    val speed = s.animSpeed.coerceIn(0.25f, 4f)
}

/**
 * niri's overview, drawn by hand:
 *  - workspaces are zoomed-out copies of the screen stacked vertically, with an empty one at the bottom;
 *  - each workspace is a horizontal strip of columns (windows) that scrolls on its own;
 *  - the focused column of each workspace wears the focus ring (bright on the active workspace);
 *  - everything moves on critically damped springs.
 *
 * Touch: tap a window to open it, swipe sideways to scroll a workspace, swipe vertically to move
 * between workspaces, swipe a window of the current workspace up to close it, long-press to
 * pick a window up and drop it on another workspace (or between two to make a new one).
 *
 * The view only knows about [OvWorkspace]/[OvColumn]; what a tap or a close actually does is
 * up to the [Host] (the launcher hook, or the preview in the settings app).
 */
class NiriOverviewView(context: Context) : View(context) {

    interface Host {
        /** A window was tapped; the zoom-in has started. Launch it now. */
        fun onLaunch(column: OvColumn)

        /** The zoom-in finished: the window fills the screen. */
        fun onLaunchVisualDone(column: OvColumn) {}

        /** A window was swiped away. */
        fun onDismiss(column: OvColumn)

        /** Empty space was tapped. */
        fun onHome()

        /** [closeToHome] finished fading out. */
        fun onCloseAnimationDone() {}

        /** Windows were moved between workspaces or closed. */
        fun onArrangementChanged(workspaces: List<OvWorkspace>, activeWorkspaceId: Int) {}
    }

    enum class Phase { IDLE, OPEN, LAUNCHING, CLOSING }

    private sealed interface Drop
    private data class Insert(val ws: Int, val slot: Int) : Drop
    private data class NewWorkspace(val after: Int) : Drop

    private enum class Mode { NONE, PENDING, STRIP, SPACES, DISMISS, DRAG }

    var host: Host? = null

    var style = OverviewStyle(NiriSettings(), resources.displayMetrics.density)
        set(value) {
            field = value
            needsSnap = true
            invalidate()
        }

    var wallpaper: Bitmap? = null
        set(value) {
            field = value
            invalidate()
        }

    /** Ignore touches that start where the system's back / home gestures live. */
    var respectSystemGestures = true
    var emptyText = "Нет недавних приложений"

    var phase = Phase.IDLE
        private set

    private val workspaces = ArrayList<OvWorkspace>()
    private var active = 0
    private var nextWsId = 1
    private var needsSnap = true
    private var pendingOpen = false
    private var openFrom: RectF? = null

    // Scene springs (niri: overview-open-close and view movement use stiffness 800,
    // workspace switching 1000, all critically damped).
    private val zoom = Spring(0.5f, 800f, 1f, 0.0002f)
    private val wsOffset = Spring(0f, 1000f, 1f, 0.0005f)
    private val sceneX = Spring(0f, 800f, 1f, 0.3f)
    private val sceneY = Spring(0f, 800f, 1f, 0.3f)
    private val sceneAlpha = Spring(1f, 1500f, 1f, 0.002f)
    private val cornerScale = Spring(1f, 800f, 1f, 0.002f)
    private val lift = Spring(0f, 900f, 1f, 0.002f)

    private var launching: OvColumn? = null
    private var launchVisualSent = false
    private var closeSent = false
    private var lastFrameNanos = 0L

    // Touch.
    private var mode = Mode.NONE
    private var downX = 0f
    private var downY = 0f
    private var lastX = 0f
    private var lastY = 0f
    private var startValue = 0f
    private var touchCol: OvColumn? = null
    private var touchWs = -1
    private var stripWs: OvWorkspace? = null
    private var velocity: VelocityTracker? = null
    private val touchSlop = ViewConfiguration.get(context).scaledTouchSlop.toFloat()
    private val longPress = Runnable { onLongPress() }

    // Drag and drop.
    private var dragCol: OvColumn? = null
    private var dragOrigin: Pair<Int, Int>? = null
    private val dragRect = RectF()
    private var dragOffX = 0f
    private var dragOffY = 0f
    private var drop: Drop? = null

    // Positions before a layout change, so things slide to their new place.
    private val prevColRects = HashMap<OvColumn, RectF>()
    private val prevWsRects = HashMap<OvWorkspace, RectF>()

    // Drawing.
    private val bitmapPaint = Paint(Paint.ANTI_ALIAS_FLAG or Paint.FILTER_BITMAP_FLAG)
    private val fill = Paint(Paint.ANTI_ALIAS_FLAG)
    private val shadow = Paint(Paint.ANTI_ALIAS_FLAG)
    private val text = TextPaint(Paint.ANTI_ALIAS_FLAG).apply { textAlign = Paint.Align.CENTER }
    private val path = Path()
    private val src = Rect()
    private val ringRect = RectF()
    private val winRect = RectF()
    private val fitRect = RectF()
    private val tmpRect = RectF()

    // ---------------------------------------------------------------- geometry

    private val w get() = width.toFloat()
    private val h get() = height.toFloat()

    /** Room kept under each window for its label (unzoomed pixels; about 22 dp once zoomed out). */
    private val labelRoom get() = if (style.labels) 22f * style.density / max(0.2f, style.zoom) else 0f

    /** Space above and below a window inside its workspace (unzoomed pixels). */
    private val vInset get() = max(style.gap * h / max(1f, w), labelRoom)

    /** Column size in unzoomed pixels: the screen minus gaps, keeping the screen's aspect. */
    private val colH get() = max(1f, h - 2f * vInset)
    private val colW get() = max(1f, colH * w / max(1f, h))
    private val colLeft get() = (w - colW) / 2f
    private val colStep get() = colW + style.gap

    /** Zoom at which one column exactly fills the screen. */
    private val fullZoom get() = w / colW

    private fun viewXFor(index: Int) = max(0, index) * colStep
    private fun maxViewX(ws: OvWorkspace) = max(0, ws.columns.size - 1) * colStep
    private fun stride(z: Float = zoom.value) = h * z + style.wsGap

    // ---------------------------------------------------------------- public API

    fun setData(spaces: List<OvWorkspace>, activeIndex: Int, focusKey: String?) {
        cancelTouch()
        workspaces.clear()
        workspaces.addAll(spaces)
        nextWsId = (workspaces.maxOfOrNull { it.id } ?: 0) + 1
        active = activeIndex
        ensureTrailingEmpty()
        active = active.coerceIn(0, workspaces.lastIndex)
        if (focusKey != null) {
            for ((i, ws) in workspaces.withIndex()) {
                val j = ws.columns.indexOfFirst { it.key == focusKey }
                if (j >= 0) {
                    ws.focus = j
                    active = i
                }
            }
        }
        for (ws in workspaces) {
            if (ws.focus !in ws.columns.indices) {
                ws.focus = ws.columns.indices.maxByOrNull { ws.columns[it].lastActive } ?: 0
            }
        }
        needsSnap = true
        invalidate()
    }

    /** Starts niri's zoom-out. [from] is where the current app is on screen right now, if known. */
    fun open(from: RectF?) {
        openFrom = from?.let { RectF(it) }
        pendingOpen = true
        phase = Phase.OPEN
        launching = null
        launchVisualSent = false
        closeSent = false
        invalidate()
    }

    /** Fades out (going home); calls [Host.onCloseAnimationDone] at the end. */
    fun closeToHome() {
        if (phase == Phase.CLOSING) return
        cancelTouch()
        phase = Phase.CLOSING
        closeSent = false
        sceneAlpha.animateTo(0f)
        zoom.animateTo(style.zoom * 1.08f)
        invalidate()
    }

    fun activeWorkspaceId(): Int = workspaces.getOrNull(active)?.id ?: 0

    fun workspacesSnapshot(): List<OvWorkspace> = workspaces.filter { it.columns.isNotEmpty() }

    /** Ends every animation at once (screenshots, tests). */
    fun snapAnimations() {
        if (needsSnap) snapViews()
        if (pendingOpen) {
            pendingOpen = false
            startOpen()
        }
        for (s in listOf(zoom, wsOffset, sceneX, sceneY, sceneAlpha, cornerScale, lift)) s.finish()
        for (ws in workspaces) {
            ws.viewX.finish()
            ws.dy.finish()
            for (c in ws.columns) {
                c.dx.finish()
                c.dy.finish()
                c.dismissY.finish()
            }
        }
        invalidate()
    }

    // ---------------------------------------------------------------- frame loop

    override fun onSizeChanged(w: Int, h: Int, oldw: Int, oldh: Int) {
        super.onSizeChanged(w, h, oldw, oldh)
        needsSnap = true
    }

    override fun onDetachedFromWindow() {
        super.onDetachedFromWindow()
        removeCallbacks(longPress)
        velocity?.recycle()
        velocity = null
    }

    override fun onDraw(canvas: Canvas) {
        if (width == 0 || height == 0) return
        if (needsSnap) snapViews()
        if (pendingOpen) {
            pendingOpen = false
            startOpen()
        }
        val now = System.nanoTime()
        val dt = if (lastFrameNanos == 0L) 1f / 60f else ((now - lastFrameNanos) / 1e9f).coerceIn(0f, 0.05f)
        lastFrameNanos = now
        val moving = step(dt * style.speed)
        layoutPass()
        drawScene(canvas)
        checkPhase()
        if (moving || mode == Mode.DRAG) postInvalidateOnAnimation() else lastFrameNanos = 0L
    }

    private fun snapViews() {
        if (width == 0) return
        needsSnap = false
        zoom.snapTo(style.zoom)
        for (ws in workspaces) ws.viewX.snapTo(viewXFor(ws.focus))
        wsOffset.snapTo(active.toFloat())
    }

    private fun startOpen() {
        val from = openFrom
        if (from != null && from.width() > 1f) {
            // Start exactly where the app is, then zoom out (niri's overview-open).
            zoom.snapTo((from.width() / colW).coerceIn(0.1f, 1.6f))
            sceneX.snapTo(from.centerX() - w / 2f)
            sceneY.snapTo(from.centerY() - h / 2f)
        } else {
            zoom.snapTo(style.zoom * 1.12f)
            sceneX.snapTo(0f)
            sceneY.snapTo(0f)
        }
        zoom.animateTo(style.zoom)
        sceneX.animateTo(0f)
        sceneY.animateTo(0f)
        sceneAlpha.snapTo(0f)
        sceneAlpha.animateTo(1f)
        cornerScale.snapTo(1f)
    }

    private fun step(dt: Float): Boolean {
        var moving = false
        for (s in listOf(zoom, wsOffset, sceneX, sceneY, sceneAlpha, cornerScale, lift)) {
            moving = s.step(dt) or moving
        }
        val finished = ArrayList<OvColumn>()
        for (ws in workspaces) {
            moving = ws.viewX.step(dt) or moving
            moving = ws.dy.step(dt) or moving
            for (c in ws.columns) {
                moving = c.dx.step(dt) or moving
                moving = c.dy.step(dt) or moving
                moving = c.dismissY.step(dt) or moving
                if (c.dismissing && (c.base.bottom + c.dismissY.value < 0f || !c.dismissY.isAnimating)) {
                    finished += c
                }
            }
        }
        dragCol?.let {
            moving = it.dx.step(dt) or moving
            moving = it.dy.step(dt) or moving
        }
        for (c in finished) finalizeRemove(c)
        if (mode == Mode.DRAG) moving = autoScroll(dt) or moving
        return moving || finished.isNotEmpty()
    }

    private fun layoutPass() {
        val z = zoom.value
        val wsW = w * z
        val wsH = h * z
        val st = wsH + style.wsGap
        val left = w / 2f - wsW / 2f + sceneX.value
        val gapAt = drop as? Insert
        for ((i, ws) in workspaces.withIndex()) {
            val cy = h / 2f + (i - wsOffset.value) * st + sceneY.value + ws.dy.value
            ws.rect.set(left, cy - wsH / 2f, left + wsW, cy + wsH / 2f)
            val top = ws.rect.top + vInset * z
            for ((j, col) in ws.columns.withIndex()) {
                val slot = if (gapAt != null && gapAt.ws == i && j >= gapAt.slot) j + 1 else j
                val x = ws.rect.left + (colLeft + slot * colStep - ws.viewX.value) * z
                col.base.set(x, top, x + colW * z, top + colH * z)
            }
        }
    }

    private fun checkPhase() {
        when (phase) {
            Phase.LAUNCHING -> if (!launchVisualSent) {
                val ws = workspaces.getOrNull(active)
                val settled = zoom.isNear(0.003f) && wsOffset.isNear(0.003f) &&
                    (ws == null || ws.viewX.isNear(1.5f))
                if (settled) {
                    launchVisualSent = true
                    launching?.let { host?.onLaunchVisualDone(it) }
                }
            }
            Phase.CLOSING -> if (!closeSent && sceneAlpha.value <= 0.02f) {
                closeSent = true
                host?.onCloseAnimationDone()
            }
            else -> Unit
        }
    }

    // ---------------------------------------------------------------- drawing

    private fun drawScene(c: Canvas) {
        val alpha = sceneAlpha.value.coerceIn(0f, 1f)
        if (alpha <= 0.003f) {
            updateHitRects()
            return
        }
        val save = if (alpha < 0.997f) c.saveLayerAlpha(0f, 0f, w, h, (alpha * 255).roundToInt()) else c.save()
        c.drawColor(style.backdrop)
        val z = zoom.value
        for (ws in workspaces) {
            if (ws.rect.bottom >= -h * 0.2f && ws.rect.top <= h * 1.2f) drawWorkspaceBackground(c, ws, z)
        }
        drawDropIndicator(c, z)
        for ((i, ws) in workspaces.withIndex()) drawColumns(c, ws, i == active, z)
        drawDragged(c, z)
        if (dragCol == null && workspaces.all { it.columns.isEmpty() }) drawEmpty(c)
        c.restoreToCount(save)
    }

    private fun updateHitRects() {
        for (ws in workspaces) for (col in ws.columns) {
            col.rect.set(col.base)
            col.rect.offset(col.dx.value, col.dy.value + col.dismissY.value)
        }
    }

    private fun drawWorkspaceBackground(c: Canvas, ws: OvWorkspace, z: Float) {
        val r = ws.rect
        val radius = style.corner * z * cornerScale.value
        if (style.shadows) {
            // niri's workspace-shadow: softness 40, spread 10, offset y 10, #00000050,
            // all relative to a 1080 px tall workspace.
            val k = r.height() / 1080f
            shadow.color = style.backdrop
            shadow.setShadowLayer(max(1f, 30f * k), 0f, 10f * k, 0x50000000)
            tmpRect.set(r)
            tmpRect.inset(-10f * k, -10f * k)
            c.drawRoundRect(tmpRect, radius, radius, shadow)
        }
        val wp = wallpaper
        if (style.useWallpaper && wp != null && !wp.isRecycled) {
            path.rewind()
            path.addRoundRect(r, radius, radius, Path.Direction.CW)
            val s = c.save()
            c.clipPath(path)
            bitmapPaint.alpha = 255
            drawBitmapCropped(c, wp, r)
            c.restoreToCount(s)
        } else {
            fill.color = style.workspaceColor
            c.drawRoundRect(r, radius, radius, fill)
        }
    }

    private fun drawColumns(c: Canvas, ws: OvWorkspace, activeWs: Boolean, z: Float) {
        val radius = style.corner * z * cornerScale.value
        val ringW = style.ring * z
        for ((j, col) in ws.columns.withIndex()) {
            col.rect.set(col.base)
            col.rect.offset(col.dx.value, col.dy.value + col.dismissY.value)
            val r = col.rect
            if (r.right < 0f || r.left > w || r.bottom < 0f || r.top > h) continue
            val fade = if (col.dismissY.value < 0f) {
                (1f + col.dismissY.value / max(1f, colH * z)).coerceIn(0f, 1f)
            } else 1f
            if (j == ws.focus && ringW > 0f) {
                drawRing(c, r, radius, ringW, if (activeWs) style.ringActive else style.ringInactive,
                    fade * cornerScale.value)
            }
            drawColumnWindows(c, col, r, radius, fade, z)
            if (style.labels && phase == Phase.OPEN) drawLabel(c, col, r, z, fade)
        }
    }

    private fun drawRing(c: Canvas, r: RectF, radius: Float, ringW: Float, color: Int, a: Float) {
        if (a <= 0.01f || Color.alpha(color) == 0) return
        fill.color = color
        fill.alpha = (Color.alpha(color) * a).roundToInt()
        ringRect.set(r)
        ringRect.inset(-ringW, -ringW)
        c.drawRoundRect(ringRect, radius + ringW, radius + ringW, fill)
    }

    private fun drawColumnWindows(c: Canvas, col: OvColumn, r: RectF, radius: Float, a: Float, z: Float) {
        val n = col.windows.size
        if (n <= 1) {
            winRect.set(r)
            drawWindow(c, col.windows.firstOrNull(), winRect, radius, a)
            return
        }
        // Split-screen pair: stacked windows in one column.
        val inner = style.gap * z * 0.5f
        val each = (r.height() - inner * (n - 1)) / n
        for ((k, win) in col.windows.withIndex()) {
            val top = r.top + k * (each + inner)
            winRect.set(r.left, top, r.right, top + each)
            drawWindow(c, win, winRect, radius, a)
        }
    }

    private fun drawWindow(c: Canvas, win: OvWindow?, r: RectF, radius: Float, a: Float) {
        path.rewind()
        path.addRoundRect(r, radius, radius, Path.Direction.CW)
        val s = c.save()
        c.clipPath(path)
        fill.color = 0xFF1C1C1C.toInt()
        fill.alpha = (255 * a).roundToInt()
        c.drawRect(r, fill)
        val bmp = win?.bitmap
        val drawable = bmp != null && !bmp.isRecycled &&
            (bmp.config != Bitmap.Config.HARDWARE || c.isHardwareAccelerated)
        if (bmp != null && drawable) {
            bitmapPaint.alpha = (255 * a).roundToInt()
            drawBitmapFitted(c, bmp, r)
        } else {
            win?.icon?.let { drawIcon(c, it, r, a) }
        }
        c.restoreToCount(s)
    }

    private fun drawIcon(c: Canvas, icon: Drawable, r: RectF, a: Float) {
        val size = min(r.width(), r.height()) * 0.28f
        val l = (r.centerX() - size / 2f).roundToInt()
        val t = (r.centerY() - size / 2f).roundToInt()
        icon.setBounds(l, t, l + size.roundToInt(), t + size.roundToInt())
        icon.alpha = (255 * a).roundToInt()
        icon.draw(c)
        icon.alpha = 255
    }

    /** Snapshots: crop to fill, but letterbox a landscape app shown in a portrait window. */
    private fun drawBitmapFitted(c: Canvas, bmp: Bitmap, dst: RectF) {
        val bw = bmp.width.toFloat()
        val bh = bmp.height.toFloat()
        if (bw < 1f || bh < 1f) return
        if ((bw > bh) != (dst.width() > dst.height())) {
            val s = min(dst.width() / bw, dst.height() / bh)
            val ww = bw * s
            val hh = bh * s
            fitRect.set(dst.centerX() - ww / 2f, dst.centerY() - hh / 2f, dst.centerX() + ww / 2f, dst.centerY() + hh / 2f)
            src.set(0, 0, bmp.width, bmp.height)
            c.drawBitmap(bmp, src, fitRect, bitmapPaint)
            return
        }
        drawBitmapCropped(c, bmp, dst)
    }

    private fun drawBitmapCropped(c: Canvas, bmp: Bitmap, dst: RectF) {
        val bw = bmp.width.toFloat()
        val bh = bmp.height.toFloat()
        if (bw < 1f || bh < 1f || dst.height() < 1f) return
        val dstAspect = dst.width() / dst.height()
        if (bw / bh > dstAspect) {
            val cw = bh * dstAspect
            val x0 = (bw - cw) / 2f
            src.set(x0.roundToInt(), 0, (x0 + cw).roundToInt(), bmp.height)
        } else {
            val ch = bw / dstAspect
            val y0 = (bh - ch) / 2f
            src.set(0, y0.roundToInt(), bmp.width, (y0 + ch).roundToInt())
        }
        c.drawBitmap(bmp, src, dst, bitmapPaint)
    }

    private fun drawLabel(c: Canvas, col: OvColumn, r: RectF, z: Float, a: Float) {
        val label = col.windows.joinToString("  |  ") { it.label }
        if (label.isBlank()) return
        val size = 12f * style.density
        text.textSize = size
        text.color = 0xFFE8E8E8.toInt()
        text.alpha = (255 * a).roundToInt()
        val shown = TextUtils.ellipsize(label, text, r.width() * 0.9f, TextUtils.TruncateAt.END).toString()
        // Centred in the margin under the window, which the layout keeps free for it.
        c.drawText(shown, r.centerX(), r.bottom + vInset * z / 2f + size * 0.35f, text)
    }

    private fun drawDropIndicator(c: Canvas, z: Float) {
        when (val d = drop) {
            is NewWorkspace -> {
                val y = if (d.after >= 0) {
                    workspaces[min(d.after, workspaces.lastIndex)].rect.bottom + style.wsGap / 2f
                } else {
                    (workspaces.firstOrNull()?.rect?.top ?: 0f) - style.wsGap / 2f
                }
                val half = w * z * 0.3f
                val t = 2f * style.density
                fill.color = style.ringActive
                fill.alpha = 255
                tmpRect.set(w / 2f - half + sceneX.value, y - t, w / 2f + half + sceneX.value, y + t)
                c.drawRoundRect(tmpRect, t, t, fill)
            }
            is Insert -> {
                val ws = workspaces.getOrNull(d.ws) ?: return
                if (ws.columns.isNotEmpty()) return
                // Dropping onto the empty workspace: outline it.
                fill.color = style.ringActive
                fill.alpha = 160
                val t = max(1f, style.ring * z)
                val radius = style.corner * z
                ringRect.set(ws.rect)
                ringRect.inset(-t, -t)
                c.drawRoundRect(ringRect, radius + t, radius + t, fill)
                fill.color = style.workspaceColor
                fill.alpha = 255
                if (!(style.useWallpaper && wallpaper != null)) c.drawRoundRect(ws.rect, radius, radius, fill)
                else drawWorkspaceBackground(c, ws, z)
            }
            null -> Unit
        }
    }

    private fun drawDragged(c: Canvas, z: Float) {
        val col = dragCol ?: return
        val s = 1f + 0.05f * lift.value
        val r = tmpRect
        r.set(dragRect)
        val cx = r.centerX()
        val cy = r.centerY()
        r.set(cx - r.width() * s / 2f, cy - r.height() * s / 2f, cx + r.width() * s / 2f, cy + r.height() * s / 2f)
        val radius = style.corner * z
        shadow.color = style.backdrop
        shadow.setShadowLayer(18f * style.density, 0f, 8f * style.density, 0x80000000.toInt())
        c.drawRoundRect(r, radius, radius, shadow)
        if (style.ring > 0f) drawRing(c, r, radius, style.ring * z, style.ringActive, 1f)
        drawColumnWindows(c, col, RectF(r), radius, 1f, z)
    }

    private fun drawEmpty(c: Canvas) {
        val ws = workspaces.getOrNull(active) ?: return
        text.textSize = 15f * style.density
        text.color = 0xFF9A9A9A.toInt()
        text.alpha = 255
        c.drawText(emptyText, ws.rect.centerX(), ws.rect.centerY(), text)
    }

    // ---------------------------------------------------------------- touch

    private fun inSystemGestureZone(x: Float, y: Float): Boolean {
        if (!respectSystemGestures) return false
        val insets = rootWindowInsets ?: return false
        val g = insets.getInsets(WindowInsets.Type.systemGestures())
        return x < g.left || x > w - g.right || y > h - g.bottom
    }

    override fun onTouchEvent(e: MotionEvent): Boolean {
        if (phase != Phase.OPEN) return phase != Phase.IDLE
        when (e.actionMasked) {
            MotionEvent.ACTION_DOWN -> {
                if (inSystemGestureZone(e.x, e.y)) {
                    mode = Mode.NONE
                    return false
                }
                velocity?.recycle()
                velocity = VelocityTracker.obtain().also { it.addMovement(e) }
                downX = e.x
                downY = e.y
                lastX = e.x
                lastY = e.y
                mode = Mode.PENDING
                val hit = hitColumn(e.x, e.y)
                touchCol = hit?.second
                touchWs = hit?.first ?: (wsIndexAt(e.y) ?: -1)
                if (touchCol != null) postDelayed(longPress, ViewConfiguration.getLongPressTimeout().toLong())
                return true
            }
            MotionEvent.ACTION_MOVE -> {
                velocity?.addMovement(e)
                onMove(e.x, e.y)
            }
            MotionEvent.ACTION_UP -> {
                velocity?.addMovement(e)
                finishGesture(e.x, e.y, cancelled = false)
            }
            MotionEvent.ACTION_CANCEL -> finishGesture(e.x, e.y, cancelled = true)
        }
        return true
    }

    private fun onMove(x: Float, y: Float) {
        lastX = x
        lastY = y
        when (mode) {
            Mode.PENDING -> {
                val dx = x - downX
                val dy = y - downY
                if (hypot(dx, dy) < touchSlop) return
                removeCallbacks(longPress)
                val col = touchCol
                if (abs(dx) > abs(dy)) {
                    val ws = workspaces.getOrNull(touchWs) ?: workspaces.getOrNull(active) ?: return
                    stripWs = ws
                    startValue = ws.viewX.value
                    ws.viewX.snapTo(startValue)
                    mode = Mode.STRIP
                } else if (col != null && touchWs == active && dy < 0f) {
                    col.dismissY.snapTo(0f)
                    mode = Mode.DISMISS
                } else {
                    startValue = wsOffset.value
                    wsOffset.snapTo(startValue)
                    mode = Mode.SPACES
                }
                downX = x
                downY = y
            }
            Mode.STRIP -> stripWs?.let { ws ->
                ws.viewX.snapTo(rubber(startValue - (x - downX) / zoom.value, 0f, maxViewX(ws)))
            }
            Mode.SPACES -> wsOffset.snapTo(
                rubber(startValue - (y - downY) / stride(), 0f, workspaces.lastIndex.toFloat()),
            )
            Mode.DISMISS -> touchCol?.let {
                val d = y - downY
                it.dismissY.snapTo(if (d < 0f) d else d * 0.2f)
            }
            Mode.DRAG -> updateDrag(x, y)
            Mode.NONE -> Unit
        }
        invalidate()
    }

    private fun finishGesture(x: Float, y: Float, cancelled: Boolean) {
        removeCallbacks(longPress)
        val vt = velocity
        vt?.computeCurrentVelocity(1000)
        val vx = if (cancelled) 0f else vt?.xVelocity ?: 0f
        val vy = if (cancelled) 0f else vt?.yVelocity ?: 0f
        when (mode) {
            Mode.PENDING -> if (!cancelled) onTap(x, y)
            Mode.STRIP -> stripWs?.let { settleStrip(it, vx) }
            Mode.SPACES -> settleSpaces(vy)
            Mode.DISMISS -> touchCol?.let { settleDismiss(it, vy, cancelled) }
            Mode.DRAG -> if (cancelled) cancelDrag() else dropDragged()
            Mode.NONE -> Unit
        }
        mode = Mode.NONE
        stripWs = null
        touchCol = null
        vt?.recycle()
        velocity = null
        invalidate()
    }

    private fun cancelTouch() {
        removeCallbacks(longPress)
        if (mode == Mode.DRAG) cancelDrag()
        if (mode == Mode.DISMISS) touchCol?.dismissY?.animateTo(0f)
        mode = Mode.NONE
        stripWs = null
        touchCol = null
    }

    private fun rubber(v: Float, lo: Float, hi: Float): Float = when {
        v < lo -> lo - (lo - v) * 0.35f
        v > hi -> hi + (v - hi) * 0.35f
        else -> v
    }

    private fun hitColumn(x: Float, y: Float): Pair<Int, OvColumn>? {
        for (i in workspaces.indices.reversed()) {
            val ws = workspaces[i]
            for (col in ws.columns.asReversed()) {
                if (!col.dismissing && col.rect.contains(x, y)) return i to col
            }
        }
        return null
    }

    private fun wsIndexAt(y: Float): Int? =
        workspaces.indexOfFirst { y >= it.rect.top && y <= it.rect.bottom }.takeIf { it >= 0 }

    override fun performClick(): Boolean = super.performClick()

    private fun onTap(x: Float, y: Float) {
        performClick()
        val hit = hitColumn(x, y)
        if (hit != null) {
            launch(hit.second)
            return
        }
        val wi = wsIndexAt(y)
        if (wi != null && wi != active) {
            active = wi
            wsOffset.animateTo(wi.toFloat())
            return
        }
        host?.onHome()
    }

    private fun launch(col: OvColumn) {
        val wi = workspaces.indexOfFirst { col in it.columns }
        if (wi < 0) return
        val ws = workspaces[wi]
        val ci = ws.columns.indexOf(col)
        active = wi
        ws.focus = ci
        phase = Phase.LAUNCHING
        launching = col
        launchVisualSent = false
        // niri closes the overview by zooming back in on the chosen window.
        wsOffset.animateTo(wi.toFloat())
        ws.viewX.animateTo(viewXFor(ci))
        zoom.animateTo(fullZoom)
        sceneX.animateTo(0f)
        sceneY.animateTo(0f)
        cornerScale.animateTo(0f)
        notifyArrangement()
        host?.onLaunch(col)
        invalidate()
    }

    private fun settleStrip(ws: OvWorkspace, vx: Float) {
        val n = ws.columns.size
        if (n == 0) {
            ws.viewX.animateTo(0f, 0f)
            return
        }
        val v = (-vx / zoom.value).coerceIn(-colStep * 8f, colStep * 8f)
        val idx = ((ws.viewX.value + v * 0.15f) / colStep).roundToInt().coerceIn(0, n - 1)
        ws.focus = idx
        ws.viewX.animateTo(viewXFor(idx), v)
    }

    private fun settleSpaces(vy: Float) {
        val v = (-vy / stride()).coerceIn(-8f, 8f)
        val idx = (wsOffset.value + v * 0.15f).roundToInt().coerceIn(0, workspaces.lastIndex)
        active = idx
        wsOffset.animateTo(idx.toFloat(), v)
    }

    private fun settleDismiss(col: OvColumn, vy: Float, cancelled: Boolean) {
        val d = col.dismissY.value
        val far = d < -colH * zoom.value * 0.3f
        val fling = vy < -600f * style.density
        if (!cancelled && d < 0f && (far || fling)) {
            col.dismissing = true
            col.dismissY.animateTo(-h, min(vy, -2000f))
            host?.onDismiss(col)
        } else {
            col.dismissY.animateTo(0f, vy)
        }
    }

    private fun finalizeRemove(col: OvColumn) {
        val wi = workspaces.indexOfFirst { col in it.columns }
        if (wi < 0) return
        captureRects()
        val ws = workspaces[wi]
        val ci = ws.columns.indexOf(col)
        ws.columns.removeAt(ci)
        prevColRects.remove(col)
        if (ci < ws.focus) ws.focus--
        ws.focus = ws.focus.coerceIn(0, max(0, ws.columns.lastIndex))
        ws.viewX.animateTo(viewXFor(ws.focus))
        compactWorkspaces()
        applyFlip()
        notifyArrangement()
    }

    // ---------------------------------------------------------------- drag and drop

    private fun onLongPress() {
        val col = touchCol ?: return
        if (mode != Mode.PENDING || phase != Phase.OPEN) return
        val wi = workspaces.indexOfFirst { col in it.columns }
        if (wi < 0) return
        val ws = workspaces[wi]
        val ci = ws.columns.indexOf(col)
        performHapticFeedback(HapticFeedbackConstants.LONG_PRESS)
        captureRects()
        mode = Mode.DRAG
        dragRect.set(col.rect)
        dragOffX = lastX - dragRect.left
        dragOffY = lastY - dragRect.top
        dragOrigin = wi to ci
        ws.columns.removeAt(ci)
        if (ci < ws.focus) ws.focus--
        ws.focus = ws.focus.coerceIn(0, max(0, ws.columns.lastIndex))
        dragCol = col
        drop = Insert(wi, ci) // keep its place open until it moves
        lift.animateTo(1f)
        applyFlip()
        invalidate()
    }

    private fun updateDrag(x: Float, y: Float) {
        dragRect.offsetTo(x - dragOffX, y - dragOffY)
        retarget(x, y)
    }

    private fun retarget(x: Float, y: Float) {
        val t = computeDrop(x, y)
        if (t != drop) {
            captureRects()
            drop = t
            applyFlip()
        }
    }

    private fun computeDrop(x: Float, y: Float): Drop? {
        if (workspaces.isEmpty()) return null
        val z = zoom.value
        val st = stride(z)
        val f = wsOffset.value + (y - h / 2f - sceneY.value) / st
        val k = f.roundToInt()
        val frac = f - k
        val half = (h * z / 2f) / st
        if (abs(frac) > half) {
            // In the gap between two workspaces: a new one goes there.
            val after = if (frac > 0f) k else k - 1
            if (after >= workspaces.lastIndex) return Insert(workspaces.lastIndex, 0)
            return NewWorkspace(after.coerceAtLeast(-1))
        }
        if (k < 0) return NewWorkspace(-1)
        if (k > workspaces.lastIndex) return Insert(workspaces.lastIndex, 0)
        val ws = workspaces[k]
        val wx = (x - ws.rect.left) / z + ws.viewX.value
        val slot = (floor((wx - colLeft - colW / 2f) / colStep).toInt() + 1).coerceIn(0, ws.columns.size)
        return Insert(k, slot)
    }

    private fun autoScroll(dt: Float): Boolean {
        var moved = false
        val edge = h * 0.12f
        val sy = when {
            lastY < edge -> -(1f - lastY / edge)
            lastY > h - edge -> 1f - (h - lastY) / edge
            else -> 0f
        }
        if (sy != 0f) {
            wsOffset.snapTo((wsOffset.value + sy * 3f * dt).coerceIn(-0.4f, workspaces.lastIndex + 0.4f))
            moved = true
        }
        val ins = drop as? Insert
        val ws = ins?.let { workspaces.getOrNull(it.ws) }
        if (ws != null) {
            val edgeX = w * 0.1f
            val sx = when {
                lastX < edgeX -> -(1f - lastX / edgeX)
                lastX > w - edgeX -> 1f - (w - lastX) / edgeX
                else -> 0f
            }
            if (sx != 0f) {
                val limit = ws.columns.size * colStep
                ws.viewX.snapTo((ws.viewX.value + sx * colStep * 2.5f * dt).coerceIn(0f, max(0f, limit)))
                moved = true
            }
        }
        if (moved) retarget(lastX, lastY)
        return moved
    }

    private fun dropDragged() {
        val col = dragCol ?: return
        captureRects()
        val t = drop
        dragCol = null
        drop = null
        var target: OvWorkspace? = null
        when (t) {
            is Insert -> workspaces.getOrNull(t.ws)?.let { ws ->
                ws.columns.add(t.slot.coerceIn(0, ws.columns.size), col)
                target = ws
            }
            is NewWorkspace -> {
                val ws = OvWorkspace(nextWsId++, mutableListOf(col))
                workspaces.add((t.after + 1).coerceIn(0, workspaces.size), ws)
                target = ws
            }
            null -> Unit
        }
        val ws = target ?: restoreOrigin(col)
        settleAfterDrop(ws, col)
    }

    private fun cancelDrag() {
        val col = dragCol ?: return
        captureRects()
        dragCol = null
        drop = null
        settleAfterDrop(restoreOrigin(col), col)
    }

    private fun restoreOrigin(col: OvColumn): OvWorkspace {
        val (wi, ci) = dragOrigin ?: (0 to 0)
        val ws = workspaces.getOrElse(wi) { workspaces.last() }
        ws.columns.add(ci.coerceIn(0, ws.columns.size), col)
        return ws
    }

    private fun settleAfterDrop(ws: OvWorkspace, col: OvColumn) {
        ws.focus = ws.columns.indexOf(col)
        active = workspaces.indexOf(ws)
        compactWorkspaces()
        ws.viewX.animateTo(viewXFor(ws.focus))
        lift.animateTo(0f)
        dragOrigin = null
        applyFlip()
        notifyArrangement()
    }

    // ---------------------------------------------------------------- structure

    private fun ensureTrailingEmpty() {
        while (workspaces.size >= 2 && workspaces.last().columns.isEmpty() &&
            workspaces[workspaces.size - 2].columns.isEmpty()
        ) {
            workspaces.removeAt(workspaces.lastIndex)
        }
        if (workspaces.isEmpty() || workspaces.last().columns.isNotEmpty()) {
            workspaces.add(OvWorkspace(nextWsId++).also { it.focus = 0 })
        }
    }

    /** Drops empty workspaces (niri does the same), keeping the one at the bottom. */
    private fun compactWorkspaces() {
        val current = workspaces.getOrNull(active)
        var i = 0
        while (i < workspaces.size - 1) {
            if (workspaces[i].columns.isEmpty()) workspaces.removeAt(i) else i++
        }
        ensureTrailingEmpty()
        val idx = current?.let { workspaces.indexOf(it) } ?: -1
        active = if (idx >= 0) idx else active.coerceIn(0, workspaces.lastIndex)
        wsOffset.snapTo(active.toFloat())
    }

    private fun captureRects() {
        prevColRects.clear()
        prevWsRects.clear()
        for (ws in workspaces) {
            prevWsRects[ws] = RectF(ws.rect)
            for (col in ws.columns) prevColRects[col] = RectF(col.rect)
        }
        dragCol?.let { prevColRects[it] = RectF(dragRect) }
    }

    /** Makes everything that moved because of a structural change slide from where it was. */
    private fun applyFlip() {
        for (ws in workspaces) ws.dy.snapTo(0f)
        layoutPass()
        for (ws in workspaces) {
            val old = prevWsRects[ws] ?: continue
            ws.dy.snapTo(old.top - ws.rect.top)
            ws.dy.animateTo(0f)
        }
        layoutPass()
        for (ws in workspaces) for (col in ws.columns) {
            val old = prevColRects[col] ?: continue
            col.dx.snapTo(old.left - col.base.left)
            col.dx.animateTo(0f)
            col.dy.snapTo(old.top - col.base.top)
            col.dy.animateTo(0f)
        }
        prevColRects.clear()
        prevWsRects.clear()
        invalidate()
    }

    private fun notifyArrangement() {
        host?.onArrangementChanged(workspacesSnapshot(), activeWorkspaceId())
    }
}
