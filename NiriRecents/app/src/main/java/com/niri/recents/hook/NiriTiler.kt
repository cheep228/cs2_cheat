package com.niri.recents.hook

import android.annotation.SuppressLint
import android.app.ActivityOptions
import android.content.Context
import android.content.Intent
import android.graphics.Rect
import android.net.Uri
import android.os.Binder
import android.os.Bundle
import android.os.Handler
import android.os.IBinder
import android.os.Looper
import android.os.Parcel
import android.os.PowerManager
import android.os.Process
import android.view.WindowInsets
import android.view.WindowManager
import com.niri.recents.overview.OvWorkspace
import com.niri.recents.shared.NiriSettings
import com.niri.recents.shared.SettingsContract
import com.niri.recents.shared.TilerProtocol
import java.lang.reflect.Proxy
import kotlin.math.roundToInt

/**
 * niri's scrolling layout for real app windows. Runs inside the launcher.
 *
 * Entering a workspace puts our desktop (an activity of the Niri Recents app that shows the
 * wallpaper and a control bar) full screen, then lays the workspace's apps over it as freeform
 * windows: columns side by side, as many as fit on screen, the rest pushed behind the desktop.
 * The bar scrolls, focuses, resizes, moves and closes columns through [binder].
 * When the desktop has been out of sight for a while, the windows go back to being normal
 * full-screen apps, so the rest of Android behaves as usual.
 */
// Holds the launcher's application context only, which lives as long as the process.
@SuppressLint("StaticFieldLeak")
internal object NiriTiler {
    private const val APP = "com.niri.recents"
    private const val DESKTOP = "com.niri.recents.ui.NiriDesktopActivity"

    class Col(val key: String, val taskIds: IntArray, val label: String, val pkg: String?)

    private class Space(val id: Int, val cols: MutableList<Col>) {
        var focus = 0
        var first = 0
    }

    private val main = Handler(Looper.getMainLooper())
    private val bg get() = Bg.handler

    @Volatile
    private var context: Context? = null
    private var store: WorkspaceStore? = null

    // Model; touched only on the background thread after enter().
    private val spaces = ArrayList<Space>()
    private var current = 0
    private val widths = HashMap<String, Float>()
    private var defaultWidth = 0.5f
    private val area = Rect()
    private var gap = 16
    private var active = false
    private var message: String? = null
    private val shown = LinkedHashSet<Int>()
    private val tiled = HashSet<Int>()
    private var listening = false
    private var reported = false

    private val restore = Runnable { giveBack() }

    /** Commands from the desktop; only the Niri Recents app may send them. */
    val binder: IBinder = object : Binder() {
        override fun onTransact(code: Int, data: Parcel, reply: Parcel?, flags: Int): Boolean {
            if (code == INTERFACE_TRANSACTION || code > 1000) return super.onTransact(code, data, reply, flags)
            if (!fromOurApp()) return false
            try {
                data.enforceInterface(TilerProtocol.DESCRIPTOR)
            } catch (t: Throwable) {
                return false
            }
            val args = IntArray(4) { if (data.dataAvail() >= 4) data.readInt() else 0 }
            bg.post { safely { command(code, args) } }
            return true
        }
    }

    private fun fromOurApp(): Boolean {
        val uid = Binder.getCallingUid()
        if (uid == Process.myUid()) return true
        return context?.packageManager?.getPackagesForUid(uid)?.contains(APP) == true
    }

    fun init(ctx: Context, loader: ClassLoader?) {
        if (context != null) return
        context = ctx.applicationContext ?: ctx
        store = WorkspaceStore(context!!)
        gap = (8f * ctx.resources.displayMetrics.density).roundToInt()
        listenToTasks(loader)
        // Windows left over from before a launcher restart go back to normal after a while.
        val leftover = store?.tiledTasks().orEmpty()
        if (leftover.isNotEmpty()) {
            bg.post { tiled.addAll(leftover) }
            bg.postDelayed(restore, 6_000)
        }
    }

    /** niri mode was switched off: every window we made goes back to full screen. */
    fun release() {
        bg.post {
            safely {
                if (tiled.isEmpty()) return@safely
                active = false
                giveBack()
            }
        }
    }

    // ---------------------------------------------------------------- entering

    /**
     * Opens workspace [wsId] as live windows with [focusKey] focused. Returns false (and the
     * caller opens the app the normal way) when freeform windows aren't available.
     */
    fun enter(ctx: Context, snapshot: List<OvWorkspace>, wsId: Int, focusKey: String, s: NiriSettings): Boolean {
        init(ctx, null)
        val app = context ?: return false
        if (!Wm.freeformSupported(app)) {
            val turnedOn = Wm.enableFreeform(app)
            Remote.report(
                app,
                if (turnedOn) "Режим niri: включил плавающие окна в системе. Перезагрузите телефон, тогда окна встанут колонками."
                else "Режим niri: плавающие окна выключены и включить не вышло (${Wm.lastError}). Включите «Плавающие окна» в настройках разработчика и перезагрузите.",
            )
            return false
        }
        val model = snapshot.filter { it.columns.isNotEmpty() }.map { ws ->
            Space(ws.id, ws.columns.map { c ->
                Col(c.key, c.taskIds, c.windows.joinToString(" · ") { it.label }, c.windows.firstOrNull()?.packageName)
            }.toMutableList())
        }
        startDesktop(app)
        bg.post {
            safely {
                val remembered = spaces.associate { it.id to (it.cols.getOrNull(it.focus)?.key to it.first) }
                spaces.clear()
                spaces.addAll(model)
                for (sp in spaces) {
                    val (key, first) = remembered[sp.id] ?: (null to 0)
                    sp.focus = sp.cols.indexOfFirst { it.key == key }.coerceAtLeast(0)
                    sp.first = first
                }
                current = spaces.indexOfFirst { it.id == wsId }.coerceAtLeast(0)
                spaces.getOrNull(current)?.let { sp -> sp.focus = sp.cols.indexOfFirst { it.key == focusKey }.coerceAtLeast(0) }
                defaultWidth = s.columnWidth.coerceIn(0.2f, 1f)
                widths.clear()
                store?.widths()?.let { widths.putAll(it) }
                if (area.isEmpty) guessArea(app)
                active = true
                message = null
                bg.removeCallbacks(restore)
                publish()
            }
        }
        // Let the desktop come up first; its HELLO lays the windows out again with exact sizes.
        bg.postDelayed({ safely { applyLayout() } }, 260)
        return true
    }

    /** Where the chosen column will be, for the overview's zoom-in. */
    fun previewBounds(snapshot: List<OvWorkspace>, wsId: Int, focusKey: String, s: NiriSettings): Rect? {
        val app = context ?: return null
        val ws = snapshot.firstOrNull { it.id == wsId } ?: return null
        val a = if (area.isEmpty) guessedArea(app) else Rect(area)
        val saved = store?.widths().orEmpty()
        val px = ws.columns.map { pxFor(saved[it.key] ?: s.columnWidth, a) }
        val focus = ws.columns.indexOfFirst { it.key == focusKey }.coerceAtLeast(0)
        val first = fitFirst(px, focus, 0, a.width())
        return rectsFor(px, first, a)[focus]
    }

    private fun startDesktop(app: Context) {
        try {
            val opts = ActivityOptions.makeBasic()
            Rx.call(opts, "setLaunchWindowingMode", Wm.FULLSCREEN)
            app.startActivity(
                Intent().setClassName(APP, DESKTOP).addFlags(Intent.FLAG_ACTIVITY_NEW_TASK),
                opts.toBundle(),
            )
        } catch (t: Throwable) {
            Remote.report(app, "Режим niri: не открылся рабочий стол: $t")
        }
    }

    // ---------------------------------------------------------------- layout

    private fun pxFor(fraction: Float, a: Rect) = (fraction.coerceIn(0.2f, 1f) * (a.width() + gap) - gap).roundToInt().coerceAtLeast(1)

    private fun widthOf(c: Col) = (widths[c.key] ?: defaultWidth).coerceIn(0.2f, 1f)

    /** Last column that fits on screen when [first] is the first one. */
    private fun lastFrom(px: List<Int>, first: Int, width: Int): Int {
        var used = 0
        var last = first
        for (i in first until px.size) {
            val need = px[i] + if (i > first) gap else 0
            if (i > first && used + need > width) break
            used += need
            last = i
        }
        return last
    }

    /** niri never centres the focus: the view moves just enough to show it, and never past the end. */
    private fun fitFirst(px: List<Int>, focus: Int, first: Int, width: Int): Int {
        if (px.isEmpty()) return 0
        var f = first.coerceIn(0, px.lastIndex)
        if (focus < f) f = focus
        while (f < focus && lastFrom(px, f, width) < focus) f++
        var maxFirst = px.lastIndex
        while (maxFirst > 0 && lastFrom(px, maxFirst - 1, width) == px.lastIndex) maxFirst--
        return minOf(f, maxFirst)
    }

    /** Screen bounds of the visible columns, by column index. A short row is centred. */
    private fun rectsFor(px: List<Int>, first: Int, a: Rect): Map<Int, Rect> {
        if (px.isEmpty()) return emptyMap()
        val last = lastFrom(px, first, a.width())
        val total = (first..last).sumOf { px[it] } + (last - first) * gap
        var x = a.left + if (first == 0 && last == px.lastIndex) maxOf(0, (a.width() - total) / 2) else 0
        val out = LinkedHashMap<Int, Rect>()
        for (i in first..last) {
            out[i] = Rect(x, a.top, x + px[i], a.bottom)
            x += px[i] + gap
        }
        return out
    }

    private fun applyLayout() {
        val app = context ?: return
        if (!active) return
        if (area.isEmpty) guessArea(app)
        val sp = spaces.getOrNull(current)
        val bounds = LinkedHashMap<Int, Rect>() // task -> window, bottom to top
        var focusTask = -1
        if (sp != null && sp.cols.isNotEmpty()) {
            val px = sp.cols.map { pxFor(widthOf(it), area) }
            sp.focus = sp.focus.coerceIn(0, sp.cols.lastIndex)
            sp.first = fitFirst(px, sp.focus, sp.first, area.width())
            val rects = rectsFor(px, sp.first, area)
            val order = rects.keys.filter { it != sp.focus } + listOfNotNull(sp.focus.takeIf { it in rects })
            for (i in order) {
                val col = sp.cols[i]
                val r = rects.getValue(i)
                // A split-screen pair shares its column, one above the other.
                val n = col.taskIds.size
                val each = (r.height() - gap * (n - 1)) / maxOf(1, n)
                for ((k, id) in col.taskIds.withIndex()) {
                    val top = r.top + k * (each + gap)
                    bounds[id] = Rect(r.left, top, r.right, top + each)
                }
            }
            focusTask = sp.cols[sp.focus].taskIds.last()
        }

        var tasks = Wm.runningTasks()
        var started = false
        for ((id, r) in bounds) {
            if (tasks[id] == null) {
                // The app's process is gone: restore it straight into its window.
                if (Wm.startFromRecents(id, Wm.FREEFORM, r)) started = true
            }
        }
        if (started) tasks = Wm.runningTasks()

        val t = Wm.Txn()
        if (t.usable) {
            tasks.values.firstOrNull { it.base?.className == DESKTOP || it.top?.className == DESKTOP }
                ?.let { t.reorder(it.token, true) }
            for (id in shown) if (id !in bounds) tasks[id]?.let { t.reorder(it.token, false) }
            for ((id, r) in bounds) {
                val ref = tasks[id] ?: continue
                if (ref.mode != Wm.FREEFORM) t.mode(ref.token, Wm.FREEFORM)
                t.bounds(ref.token, r)
                t.reorder(ref.token, true)
            }
        }
        if (!(t.usable && t.apply())) {
            val why = Wm.lastError
            for ((id, r) in bounds) {
                Wm.startFromRecents(id, Wm.FREEFORM, r)
                Wm.resizeTask(id, r)
            }
            Remote.report(app, "Режим niri: пакетная раскладка не прошла ($why), раскладываю по одному окну")
        }
        if (focusTask >= 0) Wm.setFocusedTask(focusTask)
        shown.clear()
        shown.addAll(bounds.keys)
        tiled.addAll(bounds.keys)
        store?.saveTiledTasks(tiled)
        message = if (sp == null || sp.cols.isEmpty()) "На этом столе нет окон" else null
        publish()
        if (!reported) {
            reported = true
            Remote.report(
                app,
                "Режим niri: окон на экране ${bounds.size}, область $area, " +
                    "задач запущено ${tasks.size}, последняя ошибка: ${Wm.lastError ?: "нет"}",
            )
        }
    }

    /** The desktop has been out of sight for a while: the windows become normal apps again. */
    private fun giveBack() {
        if (active || tiled.isEmpty()) return
        val app = context
        val pm = app?.getSystemService(PowerManager::class.java)
        if (pm != null && !pm.isInteractive) {
            // Screen off: the desktop will most likely come back with it.
            bg.postDelayed(restore, 5_000)
            return
        }
        val tasks = Wm.runningTasks()
        val t = Wm.Txn()
        for (id in tiled) {
            val ref = tasks[id] ?: continue
            if (ref.mode == Wm.FREEFORM) {
                t.mode(ref.token, Wm.FULLSCREEN)
                t.bounds(ref.token, Rect())
            }
        }
        if (t.apply()) {
            tiled.clear()
            shown.clear()
            store?.saveTiledTasks(emptySet())
        } else if (app != null) {
            Remote.report(app, "Режим niri: не вернул окна в полный экран: ${Wm.lastError}")
        }
    }

    private fun guessArea(app: Context) {
        area.set(guessedArea(app))
    }

    /** Screen minus status bar, navigation and the desktop's control bar. */
    private fun guessedArea(app: Context): Rect {
        val d = app.resources.displayMetrics.density
        val wm = app.getSystemService(WindowManager::class.java)
        val m = wm.currentWindowMetrics
        val b = m.bounds
        val i = m.windowInsets.getInsets(WindowInsets.Type.systemBars() or WindowInsets.Type.displayCutout())
        val bar = (TilerProtocol.BAR_DP * d).roundToInt()
        val edge = (6f * d).roundToInt()
        return Rect(b.left + edge, b.top + i.top + edge, b.right - edge, b.bottom - i.bottom - bar - edge)
    }

    // ---------------------------------------------------------------- commands

    private fun command(code: Int, a: IntArray) {
        val sp = spaces.getOrNull(current)
        when (code) {
            TilerProtocol.HELLO -> {
                val r = Rect(a[0], a[1], a[2], a[3])
                if (!r.isEmpty && r != area) {
                    area.set(r)
                    applyLayout()
                } else {
                    publish()
                }
            }
            TilerProtocol.SHOWN -> {
                bg.removeCallbacks(restore)
                if (!active) {
                    active = true
                    applyLayout()
                }
            }
            TilerProtocol.HIDDEN -> {
                active = false
                bg.removeCallbacks(restore)
                bg.postDelayed(restore, 4_000)
            }
            TilerProtocol.FOCUS -> sp?.let {
                it.focus = a[0].coerceIn(0, maxOf(0, it.cols.lastIndex))
                applyLayout()
            }
            TilerProtocol.SCROLL -> sp?.let {
                if (it.cols.isEmpty()) return
                val px = it.cols.map { c -> pxFor(widthOf(c), area) }
                it.first = a[0].coerceIn(0, it.cols.lastIndex)
                val last = lastFrom(px, it.first, area.width())
                if (it.focus < it.first) it.focus = it.first
                if (it.focus > last) it.focus = last
                applyLayout()
            }
            TilerProtocol.WIDTH -> sp?.cols?.getOrNull(sp.focus)?.let { c ->
                val now = widthOf(c)
                widths[c.key] = TilerProtocol.WIDTHS.firstOrNull { it > now + 0.01f } ?: TilerProtocol.WIDTHS[0]
                store?.saveWidths(widths)
                applyLayout()
            }
            TilerProtocol.CLOSE -> sp?.cols?.getOrNull(sp.focus)?.let { c ->
                for (id in c.taskIds) Wm.removeTask(id)
                removeTasks(c.taskIds.toSet())
                applyLayout()
            }
            TilerProtocol.MOVE -> sp?.let {
                val j = it.focus + a[0]
                if (j in it.cols.indices) {
                    it.cols.add(j, it.cols.removeAt(it.focus))
                    it.focus = j
                    saveLayout()
                    applyLayout()
                }
            }
            TilerProtocol.MOVE_WS -> sp?.cols?.getOrNull(sp.focus)?.let { c ->
                sp.cols.remove(c)
                var target = current + a[0]
                if (target < 0 || target > spaces.lastIndex) {
                    val id = (spaces.maxOfOrNull { it.id } ?: 0) + 1
                    target = if (target < 0) 0 else spaces.size
                    spaces.add(target, Space(id, mutableListOf()))
                    if (target <= current) current++
                }
                spaces[target].cols.add(c)
                spaces[target].focus = spaces[target].cols.lastIndex
                dropEmpty()
                saveLayout()
                applyLayout()
            }
            TilerProtocol.WORKSPACE -> {
                val next = (current + a[0]).coerceIn(0, maxOf(0, spaces.lastIndex))
                if (next != current) {
                    current = next
                    saveLayout()
                    applyLayout()
                }
            }
            TilerProtocol.OVERVIEW -> main.post { openOverview() }
            TilerProtocol.FULLSCREEN -> sp?.cols?.getOrNull(sp.focus)?.let { c ->
                tiled.removeAll(c.taskIds.toSet())
                shown.removeAll(c.taskIds.toSet())
                Wm.startFromRecents(c.taskIds.first(), Wm.FULLSCREEN, null)
            }
        }
    }

    private fun removeTasks(ids: Set<Int>) {
        for (sp in spaces) {
            val i = sp.cols.indexOfFirst { c -> c.taskIds.any { it in ids } }
            if (i < 0) continue
            sp.cols.removeAt(i)
            if (i < sp.focus) sp.focus--
            sp.focus = sp.focus.coerceIn(0, maxOf(0, sp.cols.lastIndex))
        }
        shown.removeAll(ids)
        tiled.removeAll(ids)
        dropEmpty()
        saveLayout()
    }

    private fun dropEmpty() {
        val cur = spaces.getOrNull(current)
        spaces.removeAll { it.cols.isEmpty() && it !== cur }
        current = spaces.indexOf(cur).let { if (it >= 0) it else 0 }
    }

    private fun saveLayout() {
        val s = store ?: return
        val id = spaces.getOrNull(current)?.id ?: return
        s.saveLayout(spaces.filter { it.cols.isNotEmpty() }.map { it.id to it.cols.map { c -> c.key } }, id)
    }

    private fun openOverview() {
        val app = context ?: return
        try {
            app.startActivity(
                Intent(Intent.ACTION_MAIN).addCategory(Intent.CATEGORY_HOME).setPackage(app.packageName)
                    .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK),
            )
        } catch (t: Throwable) {
            Remote.report(app, "Режим niri: не открылся лаунчер: $t")
            return
        }
        main.postDelayed({ Controllers.any()?.openOverview() }, 280)
    }

    /** Tells the desktop what to draw. */
    private fun publish() {
        val app = context ?: return
        val sp = spaces.getOrNull(current)
        val cols = sp?.cols.orEmpty()
        val b = Bundle()
        b.putStringArray(TilerProtocol.S_LABELS, cols.map { it.label }.toTypedArray())
        b.putStringArray(TilerProtocol.S_PACKAGES, cols.map { it.pkg ?: "" }.toTypedArray())
        b.putFloatArray(TilerProtocol.S_WIDTHS, cols.map { widthOf(it) }.toFloatArray())
        b.putInt(TilerProtocol.S_FOCUS, sp?.focus ?: 0)
        b.putInt(TilerProtocol.S_FIRST, sp?.first ?: 0)
        val last = if (sp == null || cols.isEmpty() || area.isEmpty) -1 else lastFrom(cols.map { pxFor(widthOf(it), area) }, sp.first, area.width())
        b.putInt(TilerProtocol.S_LAST, last)
        b.putInt(TilerProtocol.S_WS, current)
        b.putInt(TilerProtocol.S_WS_COUNT, spaces.size)
        b.putString(TilerProtocol.S_MESSAGE, message)
        b.putBoolean(TilerProtocol.S_ACTIVE, active)
        b.putBinder(SettingsContract.KEY_TILER_BINDER, binder)
        try {
            app.contentResolver.call(Uri.parse(SettingsContract.URI), SettingsContract.METHOD_TILER_STATE, null, b)
        } catch (t: Throwable) {
            // The app is being updated or was removed.
        }
    }

    // ---------------------------------------------------------------- task events

    /** Keeps the model right when apps close themselves or the user taps a window. */
    private fun listenToTasks(loader: ClassLoader?) {
        if (listening || loader == null) return
        val cls = Rx.cls("com.android.systemui.shared.system.TaskStackChangeListeners", loader) ?: return
        val iface = Rx.cls("com.android.systemui.shared.system.TaskStackChangeListener", loader) ?: return
        val listeners = Rx.callStatic(cls, "getInstance") ?: return
        val proxy = try {
            Proxy.newProxyInstance(iface.classLoader, arrayOf(iface)) { self, m, args ->
                when (m.name) {
                    "onTaskRemoved" -> (args?.getOrNull(0) as? Int)?.let { id ->
                        bg.post { safely { if (spaces.any { s -> s.cols.any { id in it.taskIds } }) { removeTasks(setOf(id)); if (active) applyLayout() else publish() } } }
                    }
                    "onTaskFocusChanged" -> {
                        val id = args?.getOrNull(0) as? Int
                        val focused = args?.getOrNull(1) as? Boolean ?: false
                        if (id != null && focused) bg.post { safely { onFocused(id) } }
                    }
                    "hashCode" -> return@newProxyInstance System.identityHashCode(self)
                    "equals" -> return@newProxyInstance self === args?.getOrNull(0)
                    "toString" -> return@newProxyInstance "NiriTilerTaskListener"
                }
                defaultValue(m.returnType)
            }
        } catch (t: Throwable) {
            null
        } ?: return
        if (Rx.tryCall(listeners, "registerTaskStackListener", proxy) != null) listening = true
    }

    private fun onFocused(taskId: Int) {
        if (!active) return
        val sp = spaces.getOrNull(current) ?: return
        val i = sp.cols.indexOfFirst { taskId in it.taskIds }
        if (i >= 0 && i != sp.focus) {
            sp.focus = i
            publish()
        }
    }

    private fun defaultValue(type: Class<*>): Any? = when (type) {
        java.lang.Boolean.TYPE -> false
        java.lang.Integer.TYPE -> 0
        java.lang.Long.TYPE -> 0L
        java.lang.Float.TYPE -> 0f
        java.lang.Double.TYPE -> 0.0
        else -> null
    }

    private inline fun safely(block: () -> Unit) {
        try {
            block()
        } catch (t: Throwable) {
            context?.let { Remote.report(it, "Режим niri: ошибка: " + t.stackTraceToString().take(700)) }
        }
    }
}
