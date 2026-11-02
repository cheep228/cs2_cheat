package com.niri.recents.hook

import de.robv.android.xposed.XposedHelpers

/**
 * Null-safe reflection. Launcher internals differ between Android versions and OEM forks,
 * so every lookup may fail; nothing here ever throws.
 */
internal object Rx {
    /** Holds a call's result, so "returned null" and "could not call" stay distinguishable. */
    class Result(val value: Any?)

    fun tryCall(target: Any?, name: String, vararg args: Any?): Result? {
        if (target == null) return null
        return try {
            Result(XposedHelpers.callMethod(target, name, *args))
        } catch (t: Throwable) {
            null
        }
    }

    fun call(target: Any?, name: String, vararg args: Any?): Any? = tryCall(target, name, *args)?.value

    fun callStatic(cls: Class<*>?, name: String, vararg args: Any?): Any? {
        if (cls == null) return null
        return try {
            XposedHelpers.callStaticMethod(cls, name, *args)
        } catch (t: Throwable) {
            null
        }
    }

    fun field(target: Any?, name: String): Any? {
        if (target == null) return null
        return try {
            XposedHelpers.getObjectField(target, name)
        } catch (t: Throwable) {
            null
        }
    }

    fun staticField(cls: Class<*>?, name: String): Any? {
        if (cls == null) return null
        return try {
            XposedHelpers.getStaticObjectField(cls, name)
        } catch (t: Throwable) {
            null
        }
    }

    fun cls(name: String, loader: ClassLoader?): Class<*>? = try {
        XposedHelpers.findClassIfExists(name, loader)
    } catch (t: Throwable) {
        null
    }
}
