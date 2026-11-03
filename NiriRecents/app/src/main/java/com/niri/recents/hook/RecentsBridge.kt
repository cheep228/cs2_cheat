package com.niri.recents.hook

import android.app.ActivityOptions
import android.content.Context
import android.content.Intent
import android.graphics.Bitmap
import android.graphics.ColorSpace
import android.graphics.Matrix
import android.graphics.RectF
import android.graphics.drawable.Drawable
import android.hardware.HardwareBuffer
import android.os.Handler
import android.os.Looper
import android.view.View
import android.view.ViewGroup

class TaskData(
    val id: Int,
    val label: String,
    val packageName: String?,
    val icon: Drawable?,
    val thumbnail: Bitmap?,
)

/** One TaskView of the stock Recents: a single task, or a split-screen pair. */
class ColumnData(
    val key: String,
    val taskIds: IntArray,
    val tasks: List<TaskData>,
    val lastActive: Long,
    val running: Boolean,
)

/**
 * Everything the overlay needs from the launcher's own Recents (Launcher3 Quickstep).
 * Names come from AOSP; Launcher3's ProGuard rules keep com.android.quickstep.** and
 * com.android.systemui.shared.** unobfuscated, so OEM builds such as Nothing Launcher keep them.
 */
class RecentsBridge(val recentsView: View) {
    private val loader: ClassLoader? = recentsView.javaClass.classLoader
    private val context: Context get() = recentsView.context
    private val main = Handler(Looper.getMainLooper())

    private val taskViewClass = Rx.cls("com.android.quickstep.views.TaskView", loader)
    private val launcherStateClass = Rx.cls("com.android.launcher3.LauncherState", loader)
    private val recentsStateClass = Rx.cls("com.android.quickstep.fallback.RecentsState", loader)
    private val launcherOverview = Rx.staticField(launcherStateClass, "OVERVIEW")
    private val fallbackOverview = Rx.staticField(recentsStateClass, "DEFAULT")
    private val amw: Any? by lazy {
        Rx.callStatic(Rx.cls("com.android.systemui.shared.system.ActivityManagerWrapper", loader), "getInstance")
    }

    fun describe(): String = buildString {
        append("TaskView=").append(taskViewClass != null)
        append(" LauncherState.OVERVIEW=").append(launcherOverview != null)
        append(" RecentsState.DEFAULT=").append(fallbackOverview != null)
        append(" ActivityManagerWrapper=").append(amw != null)
    }

    // ------------------------------------------------------------ state

    fun stateManager(): Any? = Rx.call(recentsView, "getStateManager")

    fun currentState(): Any? = Rx.call(stateManager(), "getState")

    /** True only for the plain Recents state, not for the in-gesture or split-select states. */
    fun isOverview(state: Any?): Boolean {
        if (state == null) return false
        if (launcherOverview != null && launcherStateClass?.isInstance(state) == true) return state === launcherOverview
        if (fallbackOverview != null && recentsStateClass?.isInstance(state) == true) return state === fallbackOverview
        return state.javaClass.simpleName == "OverviewState"
    }

    // ------------------------------------------------------------ tasks

    fun taskViews(): List<View> {
        val count = Rx.call(recentsView, "getTaskViewCount") as? Int
        if (count != null) return (0 until count).mapNotNull { Rx.call(recentsView, "getTaskViewAt", it) as? View }
        val group = recentsView as? ViewGroup ?: return emptyList()
        return (0 until group.childCount).map { group.getChildAt(it) }.filter { taskViewClass?.isInstance(it) == true }
    }

    fun columns(): List<ColumnData> {
        val out = ArrayList<ColumnData>()
        for ((index, tv) in taskViews().withIndex()) {
            val tasks = tasksOf(tv)
            val data = tasks.mapNotNull { taskData(it) }
            if (data.isEmpty()) continue
            val ids = data.map { it.id }.toIntArray()
            var last = tasks.maxOf { lastActiveOf(it) }
            // No timestamps: the stock list puts the most recent task at index 0.
            if (last <= 0L) last = -index.toLong()
            val running = Rx.call(tv, "isRunningTask") as? Boolean ?: false
            out += ColumnData(ids.sorted().joinToString("+"), ids, data, last, running)
        }
        return out
    }

    private fun tasksOf(taskView: View): List<Any> {
        val containers = Rx.call(taskView, "getTaskContainers") as? List<*>
        val fromContainers = containers?.mapNotNull { Rx.call(it, "getTask") }
        if (!fromContainers.isNullOrEmpty()) return fromContainers
        val single = Rx.call(taskView, "getFirstTask") ?: Rx.call(taskView, "getTask")
        return listOfNotNull(single)
    }

    private fun lastActiveOf(task: Any): Long = Rx.field(Rx.field(task, "key"), "lastActiveTime") as? Long ?: 0L

    private fun taskData(task: Any): TaskData? {
        val key = Rx.field(task, "key") ?: return null
        val id = Rx.field(key, "id") as? Int ?: return null
        val pkg = Rx.call(key, "getPackageName") as? String
        val title = (Rx.field(task, "title") as? String)?.takeIf { it.isNotBlank() }
            ?: pkg?.let { appLabel(it) } ?: ""
        // Copy the icon: the stock Recents still owns (and sizes) the original drawable.
        val icon = (Rx.field(task, "icon") as? Drawable)?.constantState?.newDrawable()?.mutate()
        val thumbnailData = Rx.field(task, "thumbnail")
        val thumb = (Rx.call(thumbnailData, "getThumbnail") ?: Rx.field(thumbnailData, "thumbnail")) as? Bitmap
        return TaskData(id, title, pkg, icon, thumb?.takeIf { !it.isRecycled })
    }

    private fun appLabel(pkg: String): String? = try {
        val pm = context.packageManager
        pm.getApplicationLabel(pm.getApplicationInfo(pkg, 0)).toString()
    } catch (t: Throwable) {
        null
    }

    fun loadIcon(pkg: String): Drawable? = try {
        context.packageManager.getApplicationIcon(pkg)
    } catch (t: Throwable) {
        null
    }

    /** Snapshot of a task. [fresh] re-captures it (for the app you just came from). Any thread. */
    fun loadThumbnail(taskId: Int, fresh: Boolean): Bitmap? {
        if (fresh) {
            thumbnailOf(Rx.call(amw, "takeTaskThumbnail", taskId))?.let { return it }
        }
        thumbnailOf(Rx.call(amw, "getTaskThumbnail", taskId, false))?.let { return it }
        return snapshotFromSystem(taskId)
    }

    private fun thumbnailOf(thumbnailData: Any?): Bitmap? =
        ((Rx.call(thumbnailData, "getThumbnail") ?: Rx.field(thumbnailData, "thumbnail")) as? Bitmap)
            ?.takeIf { !it.isRecycled }

    private fun snapshotFromSystem(taskId: Int): Bitmap? = try {
        val atm = Rx.callStatic(Class.forName("android.app.ActivityTaskManager"), "getService")
        val snapshot = Rx.call(atm, "getTaskSnapshot", taskId, false)
        val buffer = Rx.call(snapshot, "getHardwareBuffer") as? HardwareBuffer
        val colorSpace = Rx.call(snapshot, "getColorSpace") as? ColorSpace
        buffer?.let { Bitmap.wrapHardwareBuffer(it, colorSpace) }
    } catch (t: Throwable) {
        null
    }

    /** Where the app you came from currently sits on screen (the stock card), if any. */
    fun runningTaskRect(): RectF? {
        val tv = Rx.call(recentsView, "getRunningTaskView") as? View ?: return null
        if (!tv.isAttachedToWindow || tv.width == 0 || tv.height == 0) return null
        val target = largestChild(tv) ?: tv
        val m = Matrix()
        target.transformMatrixToGlobal(m)
        val r = RectF(0f, 0f, target.width.toFloat(), target.height.toFloat())
        m.mapRect(r)
        val loc = IntArray(2)
        recentsView.rootView.getLocationOnScreen(loc)
        r.offset(loc[0].toFloat(), loc[1].toFloat())
        return r.takeIf { it.width() > 1f && it.height() > 1f }
    }

    /** The thumbnail is the biggest child of a TaskView (the rest is the icon chip etc.). */
    private fun largestChild(v: View): View? {
        val g = v as? ViewGroup ?: return null
        var best: View? = null
        var bestArea = 0
        for (i in 0 until g.childCount) {
            val c = g.getChildAt(i)
            if (c.visibility != View.VISIBLE) continue
            val area = c.width * c.height
            if (area > bestArea) {
                bestArea = area
                best = c
            }
        }
        return best?.takeIf { bestArea > v.width * v.height / 3 }
    }

    // ------------------------------------------------------------ actions

    private fun findTaskView(taskId: Int): View? =
        Rx.call(recentsView, "getTaskViewByTaskId", taskId) as? View
            ?: taskViews().firstOrNull { tv -> (Rx.call(tv, "getTaskIds") as? IntArray)?.contains(taskId) == true }

    /**
     * Opens a task the way the stock Recents does (which also handles the live app tile),
     * and calls [onDone] once the app is on screen.
     */
    fun launch(taskIds: IntArray, onDone: () -> Unit) {
        val id = taskIds.firstOrNull() ?: return onDone()
        val tv = findTaskView(id)
        if (tv != null) {
            for (name in arrayOf("launchWithAnimation", "launchTaskAnimated", "launchTasks")) {
                val r = Rx.tryCall(tv, name) ?: continue
                val list = r.value ?: break // the launcher refused; fall back below
                if (Rx.tryCall(list, "add", Runnable { main.post { onDone() } }) == null) main.postDelayed({ onDone() }, 450)
                return
            }
        }
        val opts = ActivityOptions.makeBasic()
        val started = Rx.call(amw, "startActivityFromRecents", id, opts) as? Boolean ?: false
        if (!started) {
            val atm = Rx.callStatic(Rx.cls("android.app.ActivityTaskManager", null), "getService")
            Rx.call(atm, "startActivityFromRecents", id, opts.toBundle())
        }
        main.postDelayed({ onDone() }, 350)
    }

    fun dismiss(taskIds: IntArray) {
        val id = taskIds.firstOrNull() ?: return
        if (Rx.tryCall(recentsView, "dismissTask", id, false, true) != null) return
        findTaskView(id)?.let { tv ->
            if (Rx.tryCall(recentsView, "dismissTaskView", tv, false, true) != null) return
        }
        for (t in taskIds) Rx.call(amw, "removeTask", t)
    }

    /** Puts the launcher into its Recents state (the overlay then shows on top). */
    fun goToOverview() {
        val sm = stateManager() ?: return
        val state = launcherOverview ?: fallbackOverview ?: return
        if (Rx.tryCall(sm, "goToState", state, true) == null) Rx.tryCall(sm, "goToState", state)
    }

    fun goHome() {
        if (Rx.tryCall(recentsView, "startHome") != null) return
        try {
            context.startActivity(
                Intent(Intent.ACTION_MAIN).addCategory(Intent.CATEGORY_HOME)
                    .setPackage(context.packageName)
                    .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK),
            )
        } catch (t: Throwable) {
            // Nothing else to try.
        }
    }
}
