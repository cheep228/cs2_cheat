package com.niri.recents.overview

import android.content.Context
import android.graphics.Bitmap
import android.graphics.BitmapShader
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Matrix
import android.graphics.Paint
import android.graphics.Path
import android.graphics.RectF
import android.graphics.Shader
import android.graphics.Typeface
import android.graphics.drawable.Drawable
import android.text.TextPaint
import android.text.TextUtils
import android.view.HapticFeedbackConstants
import android.view.MotionEvent
import android.view.RoundedCorner
import android.view.VelocityTracker
import android.view.View
import android.view.ViewConfiguration
import android.view.WindowInsets
import com.niri.recents.shared.NiriSettings
import java.util.WeakHashMap
import kotlin.math.abs
import kotlin.math.floor
import kotlin.math.hypot
import kotlin.math.max
import kotlin.math.min
import kotlin.math.roundToInt

/** [NiriSettings] resolved to pixels. Every size is as seen on screen, in the overview. */
class OverviewStyle(s: NiriSettings, val density: Float) {
    /** Window size as a fraction of the screen. */
    val zoom = s.zoom.coerceIn(0.2f, 0.6f)

    /** Between two windows, and between a window and the edge of its workspace. */
    val gap = s.gapsDp.coerceIn(0f, 48f) * density
    val wsGap = s.workspaceGapDp.coerceIn(0f, 96f) * density

    /** Workspace to the side of the screen. */
    val margin = 10f * density

    /** Window corners; workspaces get matching (concentric) ones. */
    val corner = s.cornerDp.coerceIn(0f, 64f) * density
    val ring = s.ringWidthDp.coerceIn(0f, 16f) * density
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
 * niri's overview, laid out for a phone:
 *  - workspaces are cards stacked vertically, each showing the wallpaper, with a slim
 *    empty one at the bottom (niri always keeps an empty workspace last);
 *  - each workspace is a horizontal strip of windows (columns) that scrolls on its own;
 *  - the focused window of each workspace wears the focus ring, bright on the active workspace;
 *  - opening zooms out from the app you were in, tapping a window zooms back into it,
 *    and everything moves on critically damped springs.
 *
 * Touch: tap a window to open it, swipe sideways to scroll a workspace, swipe vertically to move
 * between workspaces, swipe a window of the active workspace up to close it, long-press to pick
 * a window up and drop it on another workspace (or between two, to make a new one).
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

    /**
     * Full-screen use: keep clear of the status and navigation bars, and ignore touches that
     * start where the system's back / home gestures live.
     */
    var respectSystemGestures = true

    /** Space kept free at the top and bottom when not taken from the window (previews, tests). */
    var fixedInsetTop = 0f
    var fixedInsetBottom = 0f

    var emptyText = "Нет недавних приложений"

    var phase = Phase.IDLE
        private set

    private val workspaces = ArrayList<OvWorkspace>()
    private var active = 0
    private var nextWsId = 1
    private var needsSnap = true
    private var pendingOpen = false
    private var openFrom: RectF? = null

    // Springs. niri moves the view with stiffness 800 and switches workspaces with 1000,
    // always critically damped.
    private val scrollY = Spring(0f, 1000f, 1f, 0.5f)
    private val zoomP = Spring(0f, 800f, 1f, 0.0005f)
    private val sceneScale = Spring(1f, 800f, 1f, 0.0002f)
    private val sceneAlpha = Spring(1f, 1500f, 1f, 0.002f)
    private val backAlpha = Spring(1f, 1500f, 1f, 0.002f)
    private val lift = Spring(0f, 900f, 1f, 0.002f)
    private val sceneSprings = listOf(scrollY, zoomP, sceneScale, sceneAlpha, backAlpha, lift)

    /** The window the open / launch zoom is anchored to, and where it is when [zoomP] is 1. */
    private var zoomCol: OvColumn? = null
    private val zoomTarget = RectF()

    // Scene transform, layout -> screen: screen = layout * xS + (xTx, xTy).
    private var xS = 1f
    private var xTx = 0f
    private var xTy = 0f
    private val vis = RectF()

    private var launching: OvColumn? = null
    private var launchVisualSent = false
    private var closeSent = false
    private var lastFrameNanos = 0L

    private var insetTop = 0f
    private var insetBottom = 0f
    private var displayCorner = 0f

    // Touch, in layout coordinates.
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

    // Row measurements, refreshed every layout pass.
    private var relTops = FloatArray(0)
    private var contentH = 0f
    private var spacerRel = -1f

    // Drawing.
    private val imagePaint = Paint(Paint.ANTI_ALIAS_FLAG or Paint.FILTER_BITMAP_FLAG)
    private val fill = Paint(Paint.ANTI_ALIAS_FLAG)
    private val line = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        this.style = Paint.Style.STROKE
        strokeCap = Paint.Cap.ROUND
    }
    private val shadow = Paint(Paint.ANTI_ALIAS_FLAG)
    private val text = TextPaint(Paint.ANTI_ALIAS_FLAG).apply {
        typeface = Typeface.create("sans-serif-medium", Typeface.NORMAL)
    }
    private val path = Path()
    private val shaderMatrix = Matrix()
    private val shaders = WeakHashMap<Bitmap, BitmapShader>()
    private val ringRect = RectF()
    private val winRect = RectF()
    private val anchorRect = RectF()
    private val tmpRect = RectF()
    private val dragDrawRect = RectF()

    // ---------------------------------------------------------------- geometry

    private val w get() = width.toFloat()
    private val h get() = height.toFloat()
    private val dp get() = style.density

    /** A window is the screen, scaled down. */
    private val winW get() = max(1f, w * style.zoom)
    private val winH get() = max(1f, h * style.zoom)

    /** Inside a workspace, around its windows (room for the focus ring included). */
    private val pad get() = max(style.gap, style.ring + 3f * dp)
    private val labelRoom get() = if (style.labels) 22f * dp else 0f
    private val cardLeft get() = style.margin
    private val cardW get() = max(1f, w - 2f * style.margin)
    private val fullH get() = winH + 2f * pad + labelRoom
    private val emptyH get() = min(fullH, max(64f * dp, winH * 0.32f))
    private val cardRadius get() = style.corner + pad
    private val innerW get() = max(1f, cardW - 2f * pad)
    private val step get() = winW + style.gap
    private val edge get() = max(style.wsGap, style.margin)
    private val areaTop get() = insetTop + edge
    private val areaH get() = max(1f, h - insetBottom - edge - areaTop)

    private fun rowH(ws: OvWorkspace) = emptyH + (fullH - emptyH) * ws.grow.value.coerceIn(0f, 1f)
    private fun targetH(ws: OvWorkspace) = if (ws.grow.target > 0.5f) fullH else emptyH
    private fun radiusOf(r: RectF) = min(cardRadius, r.height() / 2f)

    private fun contentW(n: Int) = if (n <= 0) 0f else n * winW + (n - 1) * style.gap
    private fun maxViewX(n: Int) = max(0f, contentW(n) - innerW)

    /** Left edge of a strip's first window when not scrolled; short strips are centred. */
    private fun stripStart(n: Int) = cardLeft + pad + max(0f, (innerW - contentW(n)) / 2f)

    /** Columns a strip lays out, counting the hole a dragged window would drop into. */
    private fun slots(i: Int, ws: OvWorkspace) = ws.columns.size + if ((drop as? Insert)?.ws == i) 1 else 0

    /** Strip scroll that shows column [j] whole, centred where possible. */
    private fun viewXFor(j: Int, n: Int) = (max(0, j) * step - (innerW - winW) / 2f).coerceIn(0f, maxViewX(n))

    private fun fullyVisible(j: Int, viewX: Float, n: Int): Boolean {
        if (maxViewX(n) <= 0.5f) return true
        val left = j * step - viewX
        return left >= -0.5f && left + winW <= innerW + 0.5f
    }

    private fun measureRows() {
        if (relTops.size != workspaces.size) relTops = FloatArray(workspaces.size)
        val nw = drop as? NewWorkspace
        var y = 0f
        spacerRel = -1f
        if (nw != null && nw.after < 0) {
            spacerRel = 0f
            y += emptyH + style.wsGap
        }
        for ((i, ws) in workspaces.withIndex()) {
            relTops[i] = y
            y += rowH(ws) + style.wsGap
            if (nw != null && nw.after == i) {
                spacerRel = y
                y += emptyH + style.wsGap
            }
        }
        contentH = max(0f, y - style.wsGap)
    }

    private fun maxScrollNow() = max(0f, contentH - areaH)

    /** Top of the first workspace when not scrolled; content shorter than the screen is centred. */
    private fun firstTop() = if (contentH <= areaH) areaTop + (areaH - contentH) / 2f else areaTop

    /** Furthest the content can scroll once running height animations end. */
    private fun targetMaxScroll(): Float {
        val total = workspaces.sumOf { targetH(it).toDouble() }.toFloat() + max(0, workspaces.size - 1) * style.wsGap
        return max(0f, total - areaH)
    }

    /** Vertical scroll that brings workspace [i] to the middle, as far as the content allows. */
    private fun scrollFor(i: Int): Float {
        if (i !in workspaces.indices) return 0f
        var top = 0f
        for (k in 0 until i) top += targetH(workspaces[k]) + style.wsGap
        return (top + targetH(workspaces[i]) / 2f - areaH / 2f).coerceIn(0f, targetMaxScroll())
    }

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
            ws.grow.snapTo(if (ws.columns.isEmpty()) 0f else 1f)
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
        backAlpha.animateTo(0f)
        sceneScale.animateTo(1.06f)
        invalidate()
    }

    fun activeWorkspaceId(): Int = workspaces.getOrNull(active)?.id ?: 0

    fun workspacesSnapshot(): List<OvWorkspace> = workspaces.filter { it.columns.isNotEmpty() }

    /** Ends every animation at once (screenshots, tests). */
    fun snapAnimations() {
        if (width == 0) return
        readInsets()
        if (needsSnap) snapViews()
        if (pendingOpen) {
            pendingOpen = false
            startOpen()
        }
        for (s in sceneSprings) s.finish()
        for (ws in workspaces) {
            ws.viewX.finish()
            ws.dy.finish()
            ws.grow.finish()
            for (c in ws.columns) {
                c.dx.finish()
                c.dy.finish()
                c.dismissY.finish()
            }
        }
        if (phase == Phase.OPEN) zoomCol = null
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
        readInsets()
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
        updateTransform()
        drawScene(canvas)
        checkPhase()
        if (moving || mode == Mode.DRAG) postInvalidateOnAnimation() else lastFrameNanos = 0L
    }

    private fun readInsets() {
        var top = fixedInsetTop
        var bottom = fixedInsetBottom
        if (respectSystemGestures) {
            rootWindowInsets?.let { wi ->
                val i = wi.getInsets(WindowInsets.Type.systemBars() or WindowInsets.Type.displayCutout())
                top = max(top, i.top.toFloat())
                bottom = max(bottom, i.bottom.toFloat())
                displayCorner = wi.getRoundedCorner(RoundedCorner.POSITION_TOP_LEFT)?.radius?.toFloat() ?: 0f
            }
        }
        if (top != insetTop || bottom != insetBottom) {
            insetTop = top
            insetBottom = bottom
            needsSnap = true
        }
    }

    private fun snapViews() {
        if (width == 0) return
        needsSnap = false
        for (ws in workspaces) {
            ws.grow.snapTo(if (ws.columns.isEmpty()) 0f else 1f)
            ws.viewX.snapTo(viewXFor(ws.focus, ws.columns.size))
        }
        measureRows()
        scrollY.snapTo(scrollFor(active))
    }

    private fun startOpen() {
        val from = openFrom
        val anchor = workspaces.getOrNull(active)?.let { it.columns.getOrNull(it.focus) }
        sceneScale.snapTo(1f)
        if (from != null && from.width() > 1f && anchor != null) {
            // Start with the app exactly where it is, then zoom out (niri's overview-open).
            zoomCol = anchor
            zoomTarget.set(from)
            zoomP.snapTo(1f)
            zoomP.animateTo(0f)
            sceneAlpha.snapTo(1f)
        } else {
            zoomCol = null
            zoomP.snapTo(0f)
            sceneScale.snapTo(1.06f)
            sceneScale.animateTo(1f)
            sceneAlpha.snapTo(0f)
            sceneAlpha.animateTo(1f)
        }
        backAlpha.snapTo(0f)
        backAlpha.animateTo(1f)
    }

    private fun step(dt: Float): Boolean {
        var moving = false
        for (s in sceneSprings) moving = s.step(dt) or moving
        val finished = ArrayList<OvColumn>()
        val top = -xTy / xS
        for (ws in workspaces) {
            moving = ws.viewX.step(dt) or moving
            moving = ws.dy.step(dt) or moving
            moving = ws.grow.step(dt) or moving
            for (c in ws.columns) {
                moving = c.dx.step(dt) or moving
                moving = c.dy.step(dt) or moving
                moving = c.dismissY.step(dt) or moving
                if (c.dismissing && (!c.dismissY.isAnimating || c.base.bottom + c.dismissY.value <= top)) finished += c
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
        measureRows()
        val gapAt = drop as? Insert
        val top0 = firstTop() - scrollY.value
        for ((i, ws) in workspaces.withIndex()) {
            val top = top0 + relTops[i] + ws.dy.value
            ws.rect.set(cardLeft, top, cardLeft + cardW, top + rowH(ws))
            val x0 = stripStart(slots(i, ws)) - ws.viewX.value
            for ((j, col) in ws.columns.withIndex()) {
                val slot = if (gapAt != null && gapAt.ws == i && j >= gapAt.slot) j + 1 else j
                val x = x0 + slot * step
                col.base.set(x, top + pad, x + winW, top + pad + winH)
                col.rect.set(col.base)
                col.rect.offset(col.dx.value, col.dy.value + col.dismissY.value)
            }
        }
    }

    /** The open / launch zoom: maps [zoomCol] from its place in the overview towards [zoomTarget]. */
    private fun updateTransform() {
        var s = 1f
        var tx = 0f
        var ty = 0f
        val col = zoomCol
        val p = zoomP.value
        if (col != null && abs(p) > 0.0001f) {
            val a = anchorRect.apply {
                set(col.base)
                offset(col.dx.value, col.dy.value)
            }
            if (a.width() > 1f) {
                val t = zoomTarget
                s = (a.width() + (t.width() - a.width()) * p) / a.width()
                tx = a.centerX() + (t.centerX() - a.centerX()) * p - a.centerX() * s
                ty = a.centerY() + (t.centerY() - a.centerY()) * p - a.centerY() * s
            }
        }
        val k = sceneScale.value
        if (k != 1f) {
            tx = (tx - w / 2f) * k + w / 2f
            ty = (ty - h / 2f) * k + h / 2f
            s *= k
        }
        xS = s
        xTx = tx
        xTy = ty
        vis.set(-xTx / xS, -xTy / xS, (w - xTx) / xS, (h - xTy) / xS)
    }

    private fun checkPhase() {
        when (phase) {
            Phase.LAUNCHING -> if (!launchVisualSent && zoomP.target == 1f && zoomP.isNear(0.002f)) {
                launchVisualSent = true
                launching?.let { host?.onLaunchVisualDone(it) }
            }
            Phase.CLOSING -> if (!closeSent && sceneAlpha.value <= 0.02f) {
                closeSent = true
                host?.onCloseAnimationDone()
            }
            Phase.OPEN -> if (zoomCol != null && !zoomP.isAnimating) zoomCol = null
            Phase.IDLE -> Unit
        }
    }

    // ---------------------------------------------------------------- drawing

    private fun drawScene(c: Canvas) {
        val back = backAlpha.value.coerceIn(0f, 1f)
        if (back > 0.003f) {
            fill.color = style.backdrop
            fill.alpha = (255 * back).roundToInt()
            c.drawRect(0f, 0f, w, h, fill)
        }
        val alpha = sceneAlpha.value.coerceIn(0f, 1f)
        if (alpha <= 0.003f) return
        val save = if (alpha < 0.997f) c.saveLayerAlpha(0f, 0f, w, h, (alpha * 255).roundToInt()) else c.save()
        c.translate(xTx, xTy)
        c.scale(xS, xS)
        drawSpacer(c)
        for ((i, ws) in workspaces.withIndex()) if (onScreen(ws.rect)) drawCard(c, ws, i)
        for ((i, ws) in workspaces.withIndex()) if (onScreen(ws.rect)) drawStrip(c, ws, i == active)
        // Windows being swiped away float above everything.
        for ((i, ws) in workspaces.withIndex()) {
            for ((j, col) in ws.columns.withIndex()) {
                if (col.dismissY.value != 0f && onScreen(col.rect)) drawColumn(c, col, j == ws.focus, i == active)
            }
        }
        drawDragged(c)
        if (dragCol == null && workspaces.all { it.columns.isEmpty() }) drawEmptyText(c)
        c.restoreToCount(save)
    }

    private fun onScreen(r: RectF) = r.right > vis.left && r.left < vis.right && r.bottom > vis.top && r.top < vis.bottom

    private fun drawCard(c: Canvas, ws: OvWorkspace, i: Int) {
        val r = ws.rect
        val radius = radiusOf(r)
        val targeted = (drop as? Insert)?.ws == i
        val placeholder = ws.columns.isEmpty() && !targeted && dragOrigin?.first != i
        val a = if (placeholder) 0.42f else 1f
        if (style.shadows && !placeholder) {
            // niri's workspace-shadow, cast by an opaque shape the card then covers.
            shadow.color = Color.BLACK
            shadow.setShadowLayer(18f * dp, 0f, 6f * dp, 0x66000000)
            c.drawRoundRect(r, radius, radius, shadow)
        }
        val wp = wallpaper
        if (style.useWallpaper && wp != null && canDraw(c, wp)) {
            drawImage(c, wp, r, radius, a, fit = false)
        } else {
            fill.color = style.workspaceColor
            fill.alpha = (255 * a).roundToInt()
            c.drawRoundRect(r, radius, radius, fill)
        }
        if (ws.columns.isEmpty() && (dragCol != null || workspaces.any { it.columns.isNotEmpty() })) {
            if (targeted) outline(c, r, radius, style.ringActive)
            drawPlus(c, r.centerX(), r.centerY(), if (targeted) style.ringActive else Color.WHITE, if (targeted) 1f else 0.55f)
        }
    }

    /** The slot a new workspace would take, while a window is dragged between two. */
    private fun drawSpacer(c: Canvas) {
        if (drop !is NewWorkspace || spacerRel < 0f) return
        val top = firstTop() - scrollY.value + spacerRel
        tmpRect.set(cardLeft, top, cardLeft + cardW, top + emptyH)
        val radius = radiusOf(tmpRect)
        fill.color = style.ringActive
        fill.alpha = 38
        c.drawRoundRect(tmpRect, radius, radius, fill)
        outline(c, tmpRect, radius, style.ringActive)
        drawPlus(c, tmpRect.centerX(), tmpRect.centerY(), style.ringActive, 1f)
    }

    private fun outline(c: Canvas, r: RectF, radius: Float, color: Int) {
        val t = max(1.5f * dp, style.ring * 0.75f)
        line.strokeWidth = t
        line.color = color
        ringRect.set(r)
        ringRect.inset(t / 2f, t / 2f)
        c.drawRoundRect(ringRect, radius - t / 2f, radius - t / 2f, line)
    }

    private fun drawPlus(c: Canvas, cx: Float, cy: Float, color: Int, a: Float) {
        val arm = 9f * dp
        line.strokeWidth = 2f * dp
        line.color = color
        line.alpha = (255 * a).roundToInt()
        c.drawLine(cx - arm, cy, cx + arm, cy, line)
        c.drawLine(cx, cy - arm, cx, cy + arm, line)
        line.alpha = 255
    }

    private fun drawStrip(c: Canvas, ws: OvWorkspace, activeWs: Boolean) {
        if (ws.columns.isEmpty()) return
        val save = c.save()
        val radius = radiusOf(ws.rect)
        path.rewind()
        path.addRoundRect(ws.rect, radius, radius, Path.Direction.CW)
        c.clipPath(path)
        for ((j, col) in ws.columns.withIndex()) {
            if (col.dismissY.value != 0f) continue
            val r = col.rect
            if (r.right < ws.rect.left || r.left > ws.rect.right || !onScreen(r)) continue
            drawColumn(c, col, j == ws.focus, activeWs)
        }
        c.restoreToCount(save)
    }

    private fun drawColumn(c: Canvas, col: OvColumn, focused: Boolean, activeWs: Boolean) {
        val r = col.rect
        val fade = if (col.dismissY.value < 0f) (1f + col.dismissY.value / (winH * 1.1f)).coerceIn(0f, 1f) else 1f
        val zoomed = if (col === zoomCol) zoomP.value.coerceIn(0f, 1f) else 0f
        val radius = windowRadius(col, r)
        if (focused && style.ring > 0f) {
            drawRing(c, r, radius, if (activeWs) style.ringActive else style.ringInactive, fade * (1f - zoomed))
        }
        drawColumnWindows(c, col, r, radius, fade)
        if (style.labels) {
            // Centred on the part of the window that shows, so a window half out of its
            // workspace doesn't get a half-cut name.
            val left = max(r.left, cardLeft + pad)
            val right = min(r.right, cardLeft + cardW - pad)
            if (right - left >= 48f * dp) drawLabel(c, col, left, right, r.bottom, fade * (1f - zoomed))
        }
    }

    /** Window corners; the window being opened grows them into the screen's own corners. */
    private fun windowRadius(col: OvColumn, r: RectF): Float {
        var radius = style.corner
        if (phase == Phase.LAUNCHING && col === zoomCol && col.base.width() > 1f) {
            val endScale = zoomTarget.width() / col.base.width()
            val end = if (endScale > 0f) displayCorner / endScale else radius
            radius += (end - radius) * zoomP.value.coerceIn(0f, 1f)
        }
        return min(radius, min(r.width(), r.height()) / 2f)
    }

    private fun drawRing(c: Canvas, r: RectF, radius: Float, color: Int, a: Float) {
        if (a <= 0.01f || Color.alpha(color) == 0) return
        val t = style.ring
        line.strokeWidth = t
        line.color = color
        line.alpha = (Color.alpha(color) * a).roundToInt()
        // Its inner edge tucks a pixel under the window, so no seam shows between the two.
        val o = t / 2f - 1f
        ringRect.set(r)
        ringRect.inset(-o, -o)
        c.drawRoundRect(ringRect, radius + o, radius + o, line)
        line.alpha = 255
    }

    private fun drawColumnWindows(c: Canvas, col: OvColumn, r: RectF, radius: Float, a: Float) {
        val n = col.windows.size
        if (n <= 1) {
            winRect.set(r)
            drawWindow(c, col.windows.firstOrNull(), winRect, radius, a)
            return
        }
        // Split-screen pair: stacked windows in one column.
        val inner = max(2f * dp, style.gap * 0.5f)
        val each = (r.height() - inner * (n - 1)) / n
        for ((k, win) in col.windows.withIndex()) {
            val top = r.top + k * (each + inner)
            winRect.set(r.left, top, r.right, top + each)
            drawWindow(c, win, winRect, min(radius, each / 2f), a)
        }
    }

    private fun drawWindow(c: Canvas, win: OvWindow?, r: RectF, radius: Float, a: Float) {
        fill.color = 0xFF1E1E1E.toInt()
        fill.alpha = (255 * a).roundToInt()
        c.drawRoundRect(r, radius, radius, fill)
        val bmp = win?.bitmap
        if (bmp != null && canDraw(c, bmp)) {
            // Snapshots fill the window; a landscape app in a portrait window is letterboxed.
            val fit = (bmp.width > bmp.height) != (r.width() > r.height())
            drawImage(c, bmp, r, radius, a, fit)
        } else {
            win?.icon?.let { drawIcon(c, it, r.centerX(), r.centerY(), min(r.width(), r.height()) * 0.3f, a) }
        }
    }

    private fun canDraw(c: Canvas, bmp: Bitmap) =
        !bmp.isRecycled && (bmp.config != Bitmap.Config.HARDWARE || c.isHardwareAccelerated)

    /** A bitmap in a rounded rectangle: cropped to fill it, or fitted inside. */
    private fun drawImage(c: Canvas, bmp: Bitmap, dst: RectF, radius: Float, a: Float, fit: Boolean) {
        val bw = bmp.width.toFloat()
        val bh = bmp.height.toFloat()
        if (bw < 1f || bh < 1f || dst.width() < 1f || dst.height() < 1f) return
        val shader = shaders.getOrPut(bmp) { BitmapShader(bmp, Shader.TileMode.DECAL, Shader.TileMode.DECAL) }
        val s = if (fit) min(dst.width() / bw, dst.height() / bh) else max(dst.width() / bw, dst.height() / bh)
        shaderMatrix.setScale(s, s)
        shaderMatrix.postTranslate(dst.centerX() - bw * s / 2f, dst.centerY() - bh * s / 2f)
        shader.setLocalMatrix(shaderMatrix)
        imagePaint.shader = shader
        imagePaint.alpha = (255 * a).roundToInt()
        c.drawRoundRect(dst, radius, radius, imagePaint)
        imagePaint.shader = null
    }

    private fun drawIcon(c: Canvas, icon: Drawable, cx: Float, cy: Float, size: Float, a: Float) {
        val l = (cx - size / 2f).roundToInt()
        val t = (cy - size / 2f).roundToInt()
        val s = size.roundToInt()
        icon.setBounds(l, t, l + s, t + s)
        if (a < 0.999f) icon.alpha = (255 * a).roundToInt()
        icon.draw(c)
        if (a < 0.999f) icon.alpha = 255
    }

    /** App icon and name, centred in the room the workspace keeps under each window. */
    private fun drawLabel(c: Canvas, col: OvColumn, left: Float, right: Float, bottom: Float, a: Float) {
        if (a <= 0.01f) return
        val label = col.windows.joinToString(" · ") { it.label }
        if (label.isBlank()) return
        text.textSize = 12f * dp
        text.textAlign = Paint.Align.LEFT
        text.color = 0xFFF0F0F0.toInt()
        text.alpha = (255 * a).roundToInt()
        text.setShadowLayer(3f * dp, 0f, 0.5f * dp, Color.argb((150 * a).roundToInt(), 0, 0, 0))
        val icon = col.windows.firstOrNull()?.icon
        val iconSize = 15f * dp
        val spacing = if (icon != null) 5f * dp else 0f
        val iconW = if (icon != null) iconSize + spacing else 0f
        val shown = TextUtils.ellipsize(label, text, max(0f, right - left - iconW), TextUtils.TruncateAt.END).toString()
        val total = iconW + text.measureText(shown)
        val cy = bottom + (pad + labelRoom) / 2f - 1f * dp
        var x = (left + right) / 2f - total / 2f
        if (icon != null) {
            drawIcon(c, icon, x + iconSize / 2f, cy, iconSize, a)
            x += iconW
        }
        c.drawText(shown, x, cy - (text.descent() + text.ascent()) / 2f, text)
        text.clearShadowLayer()
    }

    private fun drawDragged(c: Canvas) {
        val col = dragCol ?: return
        val s = 1f + 0.06f * lift.value
        val r = dragDrawRect
        val cx = dragRect.centerX()
        val cy = dragRect.centerY()
        r.set(cx - dragRect.width() * s / 2f, cy - dragRect.height() * s / 2f, cx + dragRect.width() * s / 2f, cy + dragRect.height() * s / 2f)
        val radius = style.corner * s
        shadow.color = Color.BLACK
        shadow.setShadowLayer(1f + 22f * dp * lift.value, 0f, 10f * dp * lift.value, 0x99000000.toInt())
        c.drawRoundRect(r, radius, radius, shadow)
        if (style.ring > 0f) drawRing(c, r, radius, style.ringActive, 1f)
        drawColumnWindows(c, col, r, radius, 1f)
    }

    private fun drawEmptyText(c: Canvas) {
        val ws = workspaces.lastOrNull() ?: return
        text.textSize = 14f * dp
        text.textAlign = Paint.Align.CENTER
        text.color = 0xFFB0B0B0.toInt()
        text.alpha = 255
        c.drawText(emptyText, ws.rect.centerX(), ws.rect.centerY() - (text.descent() + text.ascent()) / 2f, text)
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
        val x = (e.x - xTx) / xS
        val y = (e.y - xTy) / xS
        when (e.actionMasked) {
            MotionEvent.ACTION_DOWN -> {
                if (inSystemGestureZone(e.x, e.y)) {
                    mode = Mode.NONE
                    return false
                }
                velocity?.recycle()
                velocity = VelocityTracker.obtain().also { it.addMovement(e) }
                downX = x
                downY = y
                lastX = x
                lastY = y
                mode = Mode.PENDING
                val hit = hitColumn(x, y)
                touchCol = hit?.second
                touchWs = hit?.first ?: (wsIndexAt(x, y) ?: -1)
                if (touchCol != null) postDelayed(longPress, ViewConfiguration.getLongPressTimeout().toLong())
                return true
            }
            MotionEvent.ACTION_MOVE -> {
                velocity?.addMovement(e)
                onMove(x, y)
            }
            MotionEvent.ACTION_UP -> {
                velocity?.addMovement(e)
                finishGesture(x, y, cancelled = false)
            }
            MotionEvent.ACTION_CANCEL -> finishGesture(x, y, cancelled = true)
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
                if (hypot(dx, dy) * xS < touchSlop) return
                removeCallbacks(longPress)
                val col = touchCol
                if (abs(dx) > abs(dy)) {
                    val ws = workspaces.getOrNull(touchWs) ?: workspaces.getOrNull(active) ?: return
                    stripWs = ws
                    startValue = ws.viewX.value
                    ws.viewX.snapTo(startValue)
                    mode = Mode.STRIP
                } else if (col != null && dy < 0f && (touchWs == active || maxScrollNow() <= 0.5f)) {
                    col.dismissY.snapTo(0f)
                    mode = Mode.DISMISS
                } else {
                    startValue = scrollY.value
                    scrollY.snapTo(startValue)
                    mode = Mode.SPACES
                }
                downX = x
                downY = y
            }
            Mode.STRIP -> stripWs?.let { ws ->
                ws.viewX.snapTo(rubber(startValue - (x - downX), 0f, maxViewX(ws.columns.size)))
            }
            Mode.SPACES -> scrollY.snapTo(rubber(startValue - (y - downY), 0f, maxScrollNow()))
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
        val vx = if (cancelled) 0f else (vt?.xVelocity ?: 0f) / xS
        val vy = if (cancelled) 0f else (vt?.yVelocity ?: 0f) / xS
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
            if (!ws.rect.contains(x, y)) continue
            for (col in ws.columns.asReversed()) {
                if (!col.dismissing && col.rect.contains(x, y)) return i to col
            }
        }
        return null
    }

    private fun wsIndexAt(x: Float, y: Float): Int? =
        workspaces.indexOfFirst { it.rect.contains(x, y) }.takeIf { it >= 0 }

    override fun performClick(): Boolean = super.performClick()

    private fun onTap(x: Float, y: Float) {
        performClick()
        val hit = hitColumn(x, y)
        if (hit != null) {
            launch(hit.second)
            return
        }
        val wi = wsIndexAt(x, y)
        if (wi != null && wi != active) {
            active = wi
            scrollY.animateTo(scrollFor(wi))
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
        ws.viewX.animateTo(viewXFor(ci, ws.columns.size))
        // niri closes the overview by zooming back in on the chosen window. If the opening
        // zoom on this same window is still running, carry on from its current size.
        val a = max(1f, col.base.width())
        val shown = if (zoomCol === col && zoomP.value > 0.001f) a * xS / sceneScale.value else a
        zoomCol = col
        zoomTarget.set(0f, 0f, w, h)
        zoomP.snapTo(((shown - a) / max(1f, w - a)).coerceIn(0f, 1f))
        zoomP.animateTo(1f)
        notifyArrangement()
        host?.onLaunch(col)
        invalidate()
    }

    private fun settleStrip(ws: OvWorkspace, vx: Float) {
        val n = ws.columns.size
        val maxX = maxViewX(n)
        val v = -vx
        if (n == 0 || maxX <= 0.5f) {
            ws.viewX.animateTo(0f, v)
            return
        }
        // Snap so a window starts at the left edge, or to the very end of the strip.
        val projected = ws.viewX.value + v * 0.18f
        val target = (0 until n).map { min(it * step, maxX) }.minBy { abs(it - projected) }
        ws.viewX.animateTo(target, v)
        if (!fullyVisible(ws.focus, target, n)) {
            ws.focus = (0 until n).filter { fullyVisible(it, target, n) }.minByOrNull { abs(it - ws.focus) } ?: ws.focus
        }
        // Touching a workspace makes it the active one, as focus does in niri.
        val wi = workspaces.indexOf(ws)
        if (wi >= 0) active = wi
    }

    private fun settleSpaces(vy: Float) {
        val v = -vy
        if (maxScrollNow() <= 0.5f) {
            scrollY.animateTo(0f, v)
            return
        }
        val projected = scrollY.value + v * 0.18f
        val last = workspaces.lastIndex
        // The empty workspace at the bottom becomes active only when tapped or dropped on.
        val candidates = workspaces.indices.filter { !(it == last && last > 0 && workspaces[it].columns.isEmpty()) }
        if (candidates.isEmpty()) return
        val best = candidates.minOf { abs(scrollFor(it) - projected) }
        val near = candidates.filter { abs(scrollFor(it) - projected) <= best + 1f }
        active = when {
            v < 0f -> near.min()
            v > 0f -> near.max()
            else -> near.minBy { abs(it - active) }
        }
        scrollY.animateTo(scrollFor(active), v)
    }

    private fun settleDismiss(col: OvColumn, vy: Float, cancelled: Boolean) {
        val d = col.dismissY.value
        val far = d < -winH * 0.3f
        val fling = vy < -700f * dp
        if (!cancelled && d < 0f && (far || fling)) {
            col.dismissing = true
            col.dismissY.animateTo(-(col.base.bottom - vis.top + 40f * dp), min(vy, -1600f * dp))
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
        compactWorkspaces()
        applyFlip()
        // Stay put, unless the content got shorter than where we are.
        scrollY.animateTo(scrollY.target.coerceIn(0f, targetMaxScroll()))
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
        updateGrowTargets()
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
            updateGrowTargets()
            applyFlip()
        }
    }

    private fun computeDrop(x: Float, y: Float): Drop? {
        if (workspaces.isEmpty()) return null
        val zone = min(fullH * 0.14f, 36f * dp)
        val current = drop
        // Stay on a new-workspace slot while the finger is in it.
        if (current is NewWorkspace && spacerRel >= 0f) {
            val top = firstTop() - scrollY.value + spacerRel
            if (y >= top - style.wsGap && y <= top + emptyH + style.wsGap) return current
        }
        for ((i, ws) in workspaces.withIndex()) {
            val r = ws.rect
            val z = if (ws.columns.isEmpty()) 0f else zone
            if (y >= r.top + z && y <= r.bottom - z) {
                return Insert(i, if (ws.columns.isEmpty()) 0 else slotAt(ws, x))
            }
        }
        if (y < workspaces.first().rect.top + zone) return NewWorkspace(-1)
        for (i in 0 until workspaces.lastIndex) {
            val a = workspaces[i].rect
            val b = workspaces[i + 1].rect
            if (y >= a.bottom - zone && y <= b.top + zone) {
                val nextIsLast = i + 1 == workspaces.lastIndex && workspaces[i + 1].columns.isEmpty()
                return if (nextIsLast) Insert(i + 1, 0) else NewWorkspace(i)
            }
        }
        return Insert(workspaces.lastIndex, 0)
    }

    private fun slotAt(ws: OvWorkspace, x: Float): Int {
        val n = ws.columns.size
        val sx = x - (stripStart(n) - ws.viewX.value)
        return (floor((sx - winW / 2f) / step).toInt() + 1).coerceIn(0, n)
    }

    private fun autoScroll(dt: Float): Boolean {
        var moved = false
        val zoneY = 96f * dp
        val sy = when {
            lastY < vis.top + zoneY -> -(1f - (lastY - vis.top) / zoneY)
            lastY > vis.bottom - zoneY -> 1f - (vis.bottom - lastY) / zoneY
            else -> 0f
        }
        if (sy != 0f) {
            val v = (scrollY.value + sy.coerceIn(-1f, 1f) * 900f * dp * dt).coerceIn(0f, maxScrollNow())
            if (v != scrollY.value) {
                scrollY.snapTo(v)
                moved = true
            }
        }
        val ins = drop as? Insert
        val ws = ins?.let { workspaces.getOrNull(it.ws) }
        if (ws != null && ws.columns.isNotEmpty()) {
            val zoneX = 48f * dp
            val sx = when {
                lastX < zoneX -> -(1f - lastX / zoneX)
                lastX > w - zoneX -> 1f - (w - lastX) / zoneX
                else -> 0f
            }
            if (sx != 0f) {
                val limit = maxViewX(ws.columns.size + 1)
                val v = (ws.viewX.value + sx.coerceIn(-1f, 1f) * 700f * dp * dt).coerceIn(0f, limit)
                if (v != ws.viewX.value) {
                    ws.viewX.snapTo(v)
                    moved = true
                }
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
        dragOrigin = null
        ws.focus = ws.columns.indexOf(col)
        active = workspaces.indexOf(ws)
        compactWorkspaces()
        val n = ws.columns.size
        val keep = ws.viewX.value.coerceIn(0f, maxViewX(n))
        ws.viewX.animateTo(if (fullyVisible(ws.focus, keep, n)) keep else viewXFor(ws.focus, n))
        lift.animateTo(0f)
        applyFlip()
        scrollY.animateTo(scrollFor(active))
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
            workspaces.add(
                OvWorkspace(nextWsId++).also {
                    it.focus = 0
                    it.grow.snapTo(0f)
                },
            )
        }
    }

    /** Drops empty workspaces (niri does the same), keeping the one at the bottom. */
    private fun compactWorkspaces() {
        val current = workspaces.getOrNull(active)
        val before = active
        var i = 0
        while (i < workspaces.size - 1) {
            if (workspaces[i].columns.isEmpty()) workspaces.removeAt(i) else i++
        }
        ensureTrailingEmpty()
        val idx = current?.let { workspaces.indexOf(it) } ?: -1
        active = if (idx >= 0) idx else (before - 1).coerceIn(0, workspaces.lastIndex)
        updateGrowTargets()
    }

    /** Full height for workspaces with windows, and for any a window is about to land in. */
    private fun updateGrowTargets() {
        for ((i, ws) in workspaces.withIndex()) {
            val keep = ws.columns.isNotEmpty() || (drop as? Insert)?.ws == i ||
                (dragCol != null && dragOrigin?.first == i)
            ws.grow.animateTo(if (keep) 1f else 0f)
        }
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
        for ((i, ws) in workspaces.withIndex()) {
            ws.dy.snapTo(0f)
            // A strip that got shorter can't stay scrolled past its end.
            val maxX = maxViewX(slots(i, ws))
            val t = ws.viewX.target.coerceIn(0f, maxX)
            if (t != ws.viewX.target || ws.viewX.value !in 0f..maxX) ws.viewX.snapTo(t)
        }
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
