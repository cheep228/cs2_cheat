package com.niri.recents.hook

import android.annotation.SuppressLint
import android.app.Activity
import android.app.Application
import android.app.WallpaperManager
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.graphics.Bitmap
import android.graphics.Canvas
import android.graphics.PixelFormat
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.os.SystemClock
import android.view.View
import android.view.ViewGroup
import android.view.WindowManager
import android.graphics.RectF
import com.niri.recents.overview.ColumnKeyInfo
import com.niri.recents.overview.Md3Palette
import com.niri.recents.overview.NiriOverviewView
import com.niri.recents.overview.OverviewStyle
import com.niri.recents.overview.OvColumn
import com.niri.recents.overview.OvWindow
import com.niri.recents.overview.OvWorkspace
import com.niri.recents.overview.WorkspaceArranger
import com.niri.recents.shared.NiriSettings
import java.lang.reflect.Proxy

/**
 * Lives next to one stock RecentsView inside the launcher process. When the launcher enters
 * its Recents state, it puts a full-screen [NiriOverviewView] window on top; when the launcher
 * leaves that state (home, back, app launched), the window goes away. The stock Recents keeps
 * running underneath and does the real work: launching and closing tasks.
 */
class OverlayController(private val recentsView: View) : NiriOverviewView.Host {

    private val bridge = RecentsBridge(recentsView)
    private val context: Context = recentsView.context
    private val main = Handler(Looper.getMainLooper())
    private val store = WorkspaceStore(context)

    private var view: NiriOverviewView? = null
    private var windowManager: WindowManager? = null
    private var stateListener: Any? = null
    private var screenReceiver: BroadcastReceiver? = null

    private var launchDone = false
    private var visualDone = false
    private var generation = 0
    private var lastPhase = -1
    private var lastState: Any? = null
    private var lastEventAt = 0L
    private val seenStates = HashSet<String>()
    private var reportedShow = false

    private val watchdog = object : Runnable {
        override fun run() {
            val v = view ?: return
            if (v.phase == NiriOverviewView.Phase.OPEN || v.phase == NiriOverviewView.Phase.CLOSING) {
                if (!recentsView.isAttachedToWindow || recentsView.windowVisibility != View.VISIBLE) {
                    removeNow()
                    return
                }
                val st = bridge.currentState()
                if (st != null && !bridge.isOverview(st)) {
                    hide()
                    return
                }
            }
            main.postDelayed(this, 500)
        }
    }
    private val forceRemove = Runnable { removeNow() }
    private var showRetries = 0
    private val retryShow = Runnable {
        if (view == null && bridge.isOverview(bridge.currentState())) show()
    }

    /** Another app covered the launcher (a notification, an app opened from the shade...). */
    private val lifecycle = object : Application.ActivityLifecycleCallbacks {
        override fun onActivityStopped(activity: Activity) {
            // An app we are opening covers the launcher: let the zoom-in finish (it has a timeout).
            if (view?.phase == NiriOverviewView.Phase.LAUNCHING) {
                launchDone = true
                maybeFinishLaunch()
            } else {
                removeNow()
            }
        }

        override fun onActivityDestroyed(activity: Activity) = removeNow()
        override fun onActivityCreated(activity: Activity, savedInstanceState: Bundle?) = Unit
        override fun onActivityStarted(activity: Activity) = Unit
        override fun onActivityResumed(activity: Activity) = Unit
        override fun onActivityPaused(activity: Activity) = Unit
        override fun onActivitySaveInstanceState(activity: Activity, outState: Bundle) = Unit
    }
    private val launchTimeout = Runnable { removeNow() }

    // ------------------------------------------------------------ lifecycle

    fun attach() {
        NiriTiler.init(context, recentsView.javaClass.classLoader)
        Remote.refresh(context, force = true)
        attachStateListener()
        registerScreenOff()
        (context as? Activity)?.registerActivityLifecycleCallbacks(lifecycle)
        Remote.report(
            context,
            "Подключено: ${context.packageName} ${launcherVersion()}, Android ${Build.VERSION.RELEASE} " +
                "| ${bridge.describe()} | слушатель состояний: ${stateListener != null}",
        )
    }

    fun detach() {
        removeNow()
        try {
            (context as? Activity)?.unregisterActivityLifecycleCallbacks(lifecycle)
        } catch (t: Throwable) {
            // Ignore.
        }
        stateListener?.let { Rx.call(bridge.stateManager(), "removeStateListener", it) }
        stateListener = null
        screenReceiver?.let {
            try {
                context.unregisterReceiver(it)
            } catch (t: Throwable) {
                // Already gone.
            }
        }
        screenReceiver = null
    }

    private fun launcherVersion(): String = try {
        context.packageManager.getPackageInfo(context.packageName, 0).versionName ?: "?"
    } catch (t: Throwable) {
        "?"
    }

    /** Our own listener on the launcher's state machine, so subclass overrides can't hide events. */
    private fun attachStateListener() {
        val sm = bridge.stateManager() ?: return
        val iface = Rx.cls(
            "com.android.launcher3.statemanager.StateManager\$StateListener",
            recentsView.javaClass.classLoader,
        ) ?: return
        val proxy = try {
            Proxy.newProxyInstance(iface.classLoader, arrayOf(iface)) { self, method, args ->
                when (method.name) {
                    "onStateTransitionStart" -> {
                        onState(START, args?.getOrNull(0))
                        null
                    }
                    "onStateTransitionComplete" -> {
                        onState(COMPLETE, args?.getOrNull(0))
                        null
                    }
                    "hashCode" -> System.identityHashCode(self)
                    "equals" -> self === args?.getOrNull(0)
                    "toString" -> "NiriRecentsStateListener"
                    else -> null
                }
            }
        } catch (t: Throwable) {
            null
        } ?: return
        if (Rx.tryCall(sm, "addStateListener", proxy) != null) stateListener = proxy
    }

    private fun registerScreenOff() {
        val r = object : BroadcastReceiver() {
            override fun onReceive(c: Context, intent: Intent) = removeNow()
        }
        try {
            val filter = IntentFilter(Intent.ACTION_SCREEN_OFF)
            if (Build.VERSION.SDK_INT >= 33) {
                context.registerReceiver(r, filter, Context.RECEIVER_EXPORTED)
            } else {
                context.registerReceiver(r, filter)
            }
            screenReceiver = r
        } catch (t: Throwable) {
            // Not fatal; the watchdog still covers us.
        }
    }

    // ------------------------------------------------------------ state machine

    /** Called both by our listener and by the method hooks; duplicates are dropped. */
    fun onState(phase: Int, state: Any?) {
        val now = SystemClock.uptimeMillis()
        if (phase == lastPhase && state === lastState && now - lastEventAt < 400) return
        lastPhase = phase
        lastState = state
        lastEventAt = now
        try {
            handleState(phase, state)
        } catch (t: Throwable) {
            Remote.report(context, "Ошибка в обработке состояния: " + t.stackTraceToString().take(900))
            removeNow()
        }
    }

    private fun handleState(phase: Int, state: Any?) {
        val overview = bridge.isOverview(state)
        val name = state?.javaClass?.name ?: "null"
        if (seenStates.add("$name/$overview")) Remote.report(context, "Состояние лаунчера: $name, обзор=$overview")
        val v = view
        if (phase == START) {
            if (overview) {
                show()
            } else {
                Remote.refresh(context)
                showRetries = 0
                main.removeCallbacks(retryShow)
                if (v != null) {
                    if (v.phase == NiriOverviewView.Phase.LAUNCHING) {
                        launchDone = true
                        maybeFinishLaunch()
                    } else {
                        hide()
                    }
                }
            }
        } else {
            if (overview && view == null) {
                show()
            } else if (!overview && v != null && v.phase != NiriOverviewView.Phase.LAUNCHING) {
                hide()
            }
        }
    }

    /** The stock Recents reloaded its task list; fill in an overlay that opened too early. */
    fun onTasksReloaded() {
        val v = view ?: return
        if (v.phase == NiriOverviewView.Phase.OPEN && v.workspacesSnapshot().isEmpty() && bridge.columns().isNotEmpty()) {
            removeNow()
            show()
        }
    }

    // ------------------------------------------------------------ show / hide

    private fun show() {
        val existing = view
        if (existing != null) {
            if (existing.phase == NiriOverviewView.Phase.CLOSING) removeNow() else return
        }
        if (!recentsView.isAttachedToWindow || recentsView.windowVisibility != View.VISIBLE) {
            // Recents can be entered a moment before the launcher window turns visible.
            if (showRetries++ < 6) {
                main.removeCallbacks(retryShow)
                main.postDelayed(retryShow, 80)
            }
            return
        }
        showRetries = 0
        val settings = Remote.cached ?: NiriSettings()
        if (!settings.liveWindows) NiriTiler.release()
        if (!settings.enabled) return
        if (settings.resetToken != store.resetToken) {
            store.clear()
            store.resetToken = settings.resetToken
        }

        val data = bridge.columns()
        val running = data.firstOrNull { it.running }?.key
        val arrangement = WorkspaceArranger.arrange(
            data.map { ColumnKeyInfo(it.key, it.lastActive) }, running, store.load(), settings.perWorkspace,
        )
        store.save(arrangement.state)
        val byKey = data.associateBy { it.key }
        val spaces = arrangement.workspaces.map { (id, keys) ->
            OvWorkspace(id, keys.mapNotNull { byKey[it]?.let(::toColumn) }.toMutableList())
        }

        val (v, wm) = addOverlay(settings) ?: return
        // Track the window first, so any failure below still removes it.
        view = v
        windowManager = wm
        v.setData(spaces, arrangement.activeIndex, arrangement.focusKey)
        launchDone = false
        visualDone = false
        v.open(bridge.runningTaskRect())
        loadThumbnails(v)
        if (settings.wallpaper) loadWallpaper()
        main.removeCallbacks(watchdog)
        main.postDelayed(watchdog, 600)
        if (!reportedShow) {
            reportedShow = true
            Remote.report(context, "Niri-обзор показан: окон ${data.size}, рабочих столов ${spaces.size}")
        }
    }

    private fun addOverlay(settings: NiriSettings): Pair<NiriOverviewView, WindowManager>? {
        val contexts = listOfNotNull(overlayContext, context).distinct()
        var lastError: Throwable? = null
        for (c in contexts) {
            val v = NiriOverviewView(c)
            v.style = OverviewStyle(settings, c.resources.displayMetrics.density, Md3Palette.system(c))
            v.wallpaper = if (settings.wallpaper) cachedWallpaper else null
            v.host = this
            val wm = c.getSystemService(WindowManager::class.java) ?: continue
            try {
                wm.addView(v, layoutParams())
                return v to wm
            } catch (t: Throwable) {
                lastError = t
            }
        }
        Remote.report(context, "Не удалось показать окно обзора: $lastError")
        return null
    }

    /** A window context for overlay windows, made once per controller. */
    private val overlayContext: Context? by lazy { windowContext() }

    private fun windowContext(): Context? = try {
        // getDisplay() throws for non-visual contexts; the launcher activity is visual.
        context.createWindowContext(context.display, WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY, null)
    } catch (t: Throwable) {
        null
    }

    private fun layoutParams(): WindowManager.LayoutParams {
        val lp = WindowManager.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT,
            ViewGroup.LayoutParams.MATCH_PARENT,
            WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY,
            WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE or
                WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN or
                WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS or
                WindowManager.LayoutParams.FLAG_HARDWARE_ACCELERATED,
            PixelFormat.TRANSLUCENT,
        )
        lp.layoutInDisplayCutoutMode = WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_ALWAYS
        lp.fitInsetsTypes = 0
        lp.title = "NiriRecents"
        lp.windowAnimations = 0
        // The launcher holds SYSTEM_APPLICATION_OVERLAY: apps that hide overlays can't hide this one.
        Rx.call(lp, "setSystemApplicationOverlay", true)
        return lp
    }

    private fun hide() {
        val v = view ?: return
        when (v.phase) {
            NiriOverviewView.Phase.OPEN -> {
                v.closeToHome()
                main.removeCallbacks(forceRemove)
                main.postDelayed(forceRemove, 700)
            }
            NiriOverviewView.Phase.CLOSING -> Unit
            else -> removeNow()
        }
    }

    private fun removeNow() {
        main.removeCallbacks(watchdog)
        main.removeCallbacks(forceRemove)
        main.removeCallbacks(launchTimeout)
        val v = view ?: return
        view = null
        generation++
        try {
            windowManager?.removeViewImmediate(v)
        } catch (t: Throwable) {
            // Already removed.
        }
        windowManager = null
    }

    // ------------------------------------------------------------ data

    private fun toColumn(d: ColumnData): OvColumn = OvColumn(
        key = d.key,
        taskIds = d.taskIds,
        windows = d.tasks.map { t ->
            OvWindow(t.id, t.label, t.packageName).also {
                it.bitmap = t.thumbnail
                it.icon = t.icon
            }
        },
        lastActive = d.lastActive,
        running = d.running,
    )

    /** Snapshots: the app you came from gets a fresh one, the rest come from the system cache. */
    private fun loadThumbnails(v: NiriOverviewView) {
        val gen = generation
        val columns = v.workspacesSnapshot().flatMap { it.columns }
            .sortedByDescending { if (it.running) Long.MAX_VALUE else it.lastActive }
        Bg.handler.post {
            for (col in columns) {
                for (win in col.windows) {
                    if (gen != generation) return@post
                    if (win.bitmap == null || col.running) {
                        val bmp = bridge.loadThumbnail(win.taskId, fresh = col.running)
                        if (bmp != null) {
                            main.post {
                                if (gen == generation) {
                                    win.bitmap = bmp
                                    view?.invalidate()
                                }
                            }
                        }
                    }
                    if (win.icon == null) {
                        val icon = win.packageName?.let { bridge.loadIcon(it) }
                        if (icon != null) {
                            main.post {
                                win.icon = icon
                                view?.invalidate()
                            }
                        }
                    }
                }
            }
        }
    }

    /**
     * The workspace background. The launcher can't read the real wallpaper itself (without
     * READ_WALLPAPER_INTERNAL Android hands out the stock default one), so it comes from the
     * settings app, which copies it with root. Never fall back to someone else's picture:
     * without the real one the workspaces stay a plain colour.
     */
    @SuppressLint("MissingPermission")
    private fun loadWallpaper() {
        val stamp = Remote.wallpaperStamp
        if (cachedWallpaper != null && stamp == wallpaperStampLoaded) return
        val app = context.applicationContext ?: context
        Bg.handler.post {
            var bmp = if (stamp > 0L) Remote.loadWallpaper(app, 720) else null
            if (bmp == null) {
                val wm = WallpaperManager.getInstance(app)
                val allowed = app.checkSelfPermission("android.permission.READ_WALLPAPER_INTERNAL") ==
                    PackageManager.PERMISSION_GRANTED
                if (allowed && wm.wallpaperInfo == null) {
                    bmp = try {
                        drawableToBitmap(wm.drawable)
                    } catch (t: Throwable) {
                        null
                    }
                }
            }
            if (bmp == null && !wallpaperFailReported) {
                wallpaperFailReported = true
                Remote.report(context, "Обоев нет: откройте Niri Recents, он скопирует их через root. Пока столы цветом.")
            }
            main.post {
                cachedWallpaper = bmp
                wallpaperStampLoaded = stamp
                view?.wallpaper = bmp
            }
        }
    }

    private fun drawableToBitmap(d: android.graphics.drawable.Drawable?): Bitmap? {
        if (d == null || d.intrinsicWidth <= 0 || d.intrinsicHeight <= 0) return null
        val tw = 720
        val th = (tw.toFloat() * d.intrinsicHeight / d.intrinsicWidth).toInt().coerceIn(1, 4096)
        val out = Bitmap.createBitmap(tw, th, Bitmap.Config.ARGB_8888)
        d.setBounds(0, 0, tw, th)
        d.draw(Canvas(out))
        return out
    }

    // ------------------------------------------------------------ NiriOverviewView.Host

    override fun onLaunch(column: OvColumn) {
        launchDone = false
        visualDone = false
        main.removeCallbacks(launchTimeout)
        main.postDelayed(launchTimeout, 1_500)
        val settings = Remote.cached ?: NiriSettings()
        val v = view
        if (settings.liveWindows && v != null) {
            val spaces = v.workspacesSnapshot()
            val ws = spaces.firstOrNull { column in it.columns }
            if (ws != null && NiriTiler.enter(context, spaces, ws.id, column.key, settings)) {
                // The windows are laid out by the tiler; the overlay only finishes its zoom.
                store.setLastActive(ws.id)
                launchDone = true
                return
            }
        }
        bridge.launch(column.taskIds) {
            launchDone = true
            maybeFinishLaunch()
        }
    }

    /** In niri mode the window zooms to its column instead of the whole screen. */
    override fun launchBounds(column: OvColumn): RectF? {
        val settings = Remote.cached ?: return null
        if (!settings.liveWindows) return null
        val v = view ?: return null
        val spaces = v.workspacesSnapshot()
        val ws = spaces.firstOrNull { column in it.columns } ?: return null
        return NiriTiler.previewBounds(spaces, ws.id, column.key, settings)?.let { RectF(it) }
    }

    override fun onZoomChanged(zoom: Float) = Remote.setZoom(context, zoom)

    /** "Clear all": the stock Recents removes every task and goes home; we fade out with it. */
    override fun onDismissAll(columns: List<OvColumn>) {
        bridge.dismissAll(columns.flatMap { it.taskIds.toList() })
        main.removeCallbacks(forceRemove)
        main.postDelayed(forceRemove, 1_200)
    }

    /** Brings up Recents (used by the tiling desktop's overview button). */
    fun openOverview() {
        bridge.goToOverview()
    }

    override fun onLaunchVisualDone(column: OvColumn) {
        visualDone = true
        maybeFinishLaunch()
    }

    private fun maybeFinishLaunch() {
        if (launchDone && visualDone) removeNow()
    }

    override fun onDismiss(column: OvColumn) = bridge.dismiss(column.taskIds)

    override fun onHome() {
        view?.let { store.setLastActive(it.activeWorkspaceId()) }
        view?.closeToHome()
        main.removeCallbacks(forceRemove)
        main.postDelayed(forceRemove, 700)
        bridge.goHome()
    }

    override fun onCloseAnimationDone() = removeNow()

    override fun onArrangementChanged(workspaces: List<OvWorkspace>, activeWorkspaceId: Int) {
        store.saveLayout(workspaces.map { ws -> ws.id to ws.columns.map { it.key } }, activeWorkspaceId)
    }

    companion object {
        const val START = 0
        const val COMPLETE = 1

        @Volatile
        private var cachedWallpaper: Bitmap? = null
        private var wallpaperStampLoaded = -1L
        private var wallpaperFailReported = false
    }
}
