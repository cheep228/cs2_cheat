package com.niri.recents.hook

import android.view.View
import de.robv.android.xposed.IXposedHookLoadPackage
import de.robv.android.xposed.XC_MethodHook
import de.robv.android.xposed.XC_MethodReplacement
import de.robv.android.xposed.XposedBridge
import de.robv.android.xposed.XposedHelpers
import de.robv.android.xposed.callbacks.XC_LoadPackage
import java.util.WeakHashMap

/** Entry point listed in assets/xposed_init. */
class HookEntry : IXposedHookLoadPackage {

    override fun handleLoadPackage(lpparam: XC_LoadPackage.LoadPackageParam) {
        if (lpparam.packageName == SELF) {
            // Lets the settings app show "module active".
            try {
                XposedHelpers.findAndHookMethod(
                    "com.niri.recents.ModuleStatus", lpparam.classLoader, "isActive",
                    XC_MethodReplacement.returnConstant(true),
                )
            } catch (t: Throwable) {
                XposedBridge.log(t)
            }
            return
        }
        if (!lpparam.isFirstApplication) return
        // Only launchers that host Android's Recents (Quickstep) have this class.
        val recentsView = Rx.cls("com.android.quickstep.views.RecentsView", lpparam.classLoader) ?: return
        try {
            RecentsHooks.install(lpparam.classLoader, recentsView)
            XposedBridge.log("NiriRecents: hooks installed in ${lpparam.packageName}")
        } catch (t: Throwable) {
            XposedBridge.log("NiriRecents: install failed in ${lpparam.packageName}: $t")
        }
    }

    private companion object {
        const val SELF = "com.niri.recents"
    }
}

internal object RecentsHooks {

    fun install(loader: ClassLoader, recentsViewClass: Class<*>) {
        val attach = after { p -> (p.thisObject as? View)?.let { Controllers.attach(it) } }
        val detach = after { p -> (p.thisObject as? View)?.let { Controllers.detach(it) } }
        if (!hookDeclared(recentsViewClass, "onAttachedToWindow", attach)) {
            XposedBridge.hookAllMethods(View::class.java, "onAttachedToWindow", after { p ->
                if (recentsViewClass.isInstance(p.thisObject)) Controllers.attach(p.thisObject as View)
            })
        }
        if (!hookDeclared(recentsViewClass, "onDetachedFromWindow", detach)) {
            XposedBridge.hookAllMethods(View::class.java, "onDetachedFromWindow", after { p ->
                if (recentsViewClass.isInstance(p.thisObject)) Controllers.detach(p.thisObject as View)
            })
        }
        XposedBridge.hookAllMethods(recentsViewClass, "applyLoadPlan", after { p ->
            Controllers.of(p.thisObject)?.onTasksReloaded()
        })
        // Backup for our own StateListener: the RecentsView implementations get every transition.
        for (name in listOf(
            "com.android.quickstep.views.LauncherRecentsView",
            "com.android.quickstep.fallback.FallbackRecentsView",
        )) {
            val c = Rx.cls(name, loader) ?: continue
            XposedBridge.hookAllMethods(c, "onStateTransitionStart", after { p ->
                Controllers.forView(p.thisObject)?.onState(OverlayController.START, p.args.firstOrNull())
            })
            XposedBridge.hookAllMethods(c, "onStateTransitionComplete", after { p ->
                Controllers.forView(p.thisObject)?.onState(OverlayController.COMPLETE, p.args.firstOrNull())
            })
        }
    }

    private fun hookDeclared(cls: Class<*>, name: String, hook: XC_MethodHook): Boolean {
        val m = cls.declaredMethods.firstOrNull { it.name == name && it.parameterTypes.isEmpty() } ?: return false
        XposedBridge.hookMethod(m, hook)
        return true
    }

    private fun after(block: (XC_MethodHook.MethodHookParam) -> Unit) = object : XC_MethodHook() {
        override fun afterHookedMethod(param: MethodHookParam) {
            try {
                block(param)
            } catch (t: Throwable) {
                XposedBridge.log(t)
            }
        }
    }
}

/** One controller per RecentsView instance. */
internal object Controllers {
    private val map = WeakHashMap<View, OverlayController>()

    fun attach(view: View) {
        if (map[view] != null) return
        val c = OverlayController(view)
        map[view] = c
        c.attach()
    }

    fun detach(view: View) {
        map.remove(view)?.detach()
    }

    fun of(obj: Any?): OverlayController? = (obj as? View)?.let { map[it] }

    fun forView(obj: Any?): OverlayController? {
        val v = obj as? View ?: return null
        map[v]?.let { return it }
        attach(v)
        return map[v]
    }
}
