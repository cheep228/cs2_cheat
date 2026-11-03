package com.niri.recents.hook

import android.app.ActivityOptions
import android.content.ComponentName
import android.content.Context
import android.content.pm.PackageManager
import android.graphics.Rect
import android.provider.Settings
import java.lang.reflect.InvocationTargetException

/**
 * Window-manager calls for the tiling mode. They run inside the launcher, which holds
 * MANAGE_ACTIVITY_TASKS and START_TASKS_FROM_RECENTS (Launcher3 Quickstep asks for both), so
 * it may move, resize and restack other apps' tasks. Nothing here throws; failures are kept
 * in [lastError] for the diagnostics.
 */
internal object Wm {
    const val FULLSCREEN = 1
    const val FREEFORM = 5

    @Volatile
    var lastError: String? = null

    class TaskRef(
        val id: Int,
        val token: Any?,
        val mode: Int,
        val visible: Boolean,
        val base: ComponentName?,
        val top: ComponentName?,
    )

    private fun <T> attempt(what: String, block: () -> T): T? = try {
        block()
    } catch (t: Throwable) {
        val cause = (t as? InvocationTargetException)?.targetException ?: t
        lastError = "$what: ${cause.javaClass.simpleName}: ${cause.message}"
        null
    }

    private val atmClass: Class<*>? by lazy { attempt("ActivityTaskManager") { Class.forName("android.app.ActivityTaskManager") } }

    private fun service(): Any? = attempt("ActivityTaskManager.getService") { atmClass?.getMethod("getService")?.invoke(null) }

    /** Freeform windows exist only if the device declares them or the developer option is on. */
    fun freeformSupported(context: Context): Boolean =
        context.packageManager.hasSystemFeature(PackageManager.FEATURE_FREEFORM_WINDOW_MANAGEMENT) ||
            Settings.Global.getInt(context.contentResolver, "enable_freeform_support", 0) != 0

    /** Turns on freeform windows (the launcher may write secure settings). Needs a reboot to apply. */
    fun enableFreeform(context: Context): Boolean = attempt("enable freeform") {
        Settings.Global.putInt(context.contentResolver, "enable_freeform_support", 1)
        Settings.Global.putInt(context.contentResolver, "force_resizable_activities", 1)
        true
    } ?: false

    /** Running tasks by id, with the tokens window-container transactions need. */
    fun runningTasks(): Map<Int, TaskRef> {
        val list = attempt("getTasks") {
            val cls = atmClass ?: return@attempt null
            val inst = cls.getMethod("getInstance").invoke(null)
            cls.getMethod("getTasks", Int::class.javaPrimitiveType).invoke(inst, 200) as? List<*>
        } ?: return emptyMap()
        val out = HashMap<Int, TaskRef>()
        for (info in list) {
            info ?: continue
            val id = Rx.field(info, "taskId") as? Int ?: continue
            out[id] = TaskRef(
                id = id,
                token = Rx.field(info, "token"),
                mode = Rx.call(info, "getWindowingMode") as? Int ?: 0,
                visible = Rx.field(info, "isVisible") as? Boolean ?: false,
                base = Rx.field(info, "baseActivity") as? ComponentName,
                top = Rx.field(info, "topActivity") as? ComponentName,
            )
        }
        return out
    }

    /** Brings a task (restoring it if its process is gone) to the front in [mode]. */
    fun startFromRecents(taskId: Int, mode: Int, bounds: Rect?): Boolean = attempt("startActivityFromRecents($taskId)") {
        val opts = ActivityOptions.makeBasic()
        opts.javaClass.getMethod("setLaunchWindowingMode", Int::class.javaPrimitiveType).invoke(opts, mode)
        if (bounds != null) opts.setLaunchBounds(bounds)
        val svc = service() ?: return@attempt false
        val r = svc.javaClass.getMethod("startActivityFromRecents", Int::class.javaPrimitiveType, android.os.Bundle::class.java)
            .invoke(svc, taskId, opts.toBundle()) as? Int ?: -1
        r >= 0
    } ?: false

    fun setFocusedTask(taskId: Int): Boolean = attempt("setFocusedTask") {
        val svc = service() ?: return@attempt false
        svc.javaClass.getMethod("setFocusedTask", Int::class.javaPrimitiveType).invoke(svc, taskId)
        true
    } ?: false

    fun removeTask(taskId: Int): Boolean = attempt("removeTask") {
        val svc = service() ?: return@attempt false
        svc.javaClass.getMethod("removeTask", Int::class.javaPrimitiveType).invoke(svc, taskId) as? Boolean ?: false
    } ?: false

    fun resizeTask(taskId: Int, bounds: Rect): Boolean = attempt("resizeTask") {
        val svc = service() ?: return@attempt false
        svc.javaClass.getMethod("resizeTask", Int::class.javaPrimitiveType, Rect::class.java, Int::class.javaPrimitiveType)
            .invoke(svc, taskId, bounds, 0)
        true
    } ?: false

    /** A batch of window changes, applied at once. */
    class Txn {
        private val cls: Class<*>? = attempt("WindowContainerTransaction") { Class.forName("android.window.WindowContainerTransaction") }
        private val tokenCls: Class<*>? = attempt("WindowContainerToken") { Class.forName("android.window.WindowContainerToken") }
        private val wct: Any? = attempt("new WindowContainerTransaction") { cls?.getConstructor()?.newInstance() }
        var ops = 0
            private set

        val usable get() = wct != null && tokenCls != null

        fun mode(token: Any?, mode: Int) = op("setWindowingMode", token) {
            cls!!.getMethod("setWindowingMode", tokenCls, Int::class.javaPrimitiveType).invoke(wct, token, mode)
        }

        fun bounds(token: Any?, rect: Rect) = op("setBounds", token) {
            cls!!.getMethod("setBounds", tokenCls, Rect::class.java).invoke(wct, token, rect)
        }

        fun reorder(token: Any?, onTop: Boolean) = op("reorder", token) {
            cls!!.getMethod("reorder", tokenCls, Boolean::class.javaPrimitiveType).invoke(wct, token, onTop)
        }

        private fun op(what: String, token: Any?, block: () -> Unit) {
            if (token == null || !usable) return
            if (attempt(what) { block() } != null) ops++
        }

        /** Applies the batch; true if the system took it. */
        fun apply(): Boolean {
            if (!usable || ops == 0) return ops == 0
            return attempt("applyTransaction") {
                val org = Class.forName("android.window.WindowOrganizer")
                org.getMethod("applyTransaction", cls).invoke(org.getConstructor().newInstance(), wct)
                true
            } ?: false
        }
    }
}
