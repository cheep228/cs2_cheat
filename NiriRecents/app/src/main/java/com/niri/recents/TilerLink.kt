package com.niri.recents

import android.os.Bundle
import android.os.Handler
import android.os.IBinder
import android.os.Looper
import android.os.Parcel
import com.niri.recents.shared.TilerProtocol
import java.util.concurrent.CopyOnWriteArrayList

/**
 * The app's end of the tiling channel. The launcher hands over its command [binder] and the
 * current layout through [SettingsProvider]; the desktop draws the layout and sends commands.
 */
object TilerLink {
    @Volatile
    var binder: IBinder? = null
        private set

    @Volatile
    var state: Bundle? = null
        private set

    private val main = Handler(Looper.getMainLooper())
    private val listeners = CopyOnWriteArrayList<() -> Unit>()

    fun offerBinder(b: IBinder?) {
        if (b != null && b.isBinderAlive) binder = b
    }

    fun update(b: Bundle) {
        state = b
        main.post { listeners.forEach { it() } }
    }

    fun listen(l: () -> Unit) {
        listeners += l
    }

    fun unlisten(l: () -> Unit) {
        listeners -= l
    }

    /** Sends a command; false if the launcher isn't reachable (not hooked yet, or restarted). */
    fun send(code: Int, vararg args: Int): Boolean {
        val b = binder ?: return false
        val data = Parcel.obtain()
        return try {
            data.writeInterfaceToken(TilerProtocol.DESCRIPTOR)
            for (a in args) data.writeInt(a)
            b.transact(code, data, null, IBinder.FLAG_ONEWAY)
        } catch (t: Throwable) {
            binder = null
            false
        } finally {
            data.recycle()
        }
    }
}
