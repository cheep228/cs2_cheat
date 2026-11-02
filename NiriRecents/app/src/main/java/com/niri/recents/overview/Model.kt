package com.niri.recents.overview

import android.graphics.Bitmap
import android.graphics.RectF
import android.graphics.drawable.Drawable

/** One app window: a recent task and its snapshot. */
class OvWindow(val taskId: Int, val label: String, val packageName: String?) {
    @Volatile
    var bitmap: Bitmap? = null

    @Volatile
    var icon: Drawable? = null
}

/**
 * A niri column. Usually one window; a split-screen pair becomes a column with two
 * windows stacked on top of each other, like stacked windows in niri.
 */
class OvColumn(
    val key: String,
    val taskIds: IntArray,
    val windows: List<OvWindow>,
    val lastActive: Long,
    val running: Boolean = false,
) {
    /** Where the layout puts it this frame. */
    internal val base = RectF()

    /** Where it was actually drawn (base + animated offsets); used for hit testing. */
    internal val rect = RectF()

    // Offsets that decay to zero, so layout changes slide instead of jumping.
    internal val dx = Spring(0f, 800f, 1f, 0.5f)
    internal val dy = Spring(0f, 800f, 1f, 0.5f)

    /** Swipe-up-to-close offset. */
    internal val dismissY = Spring(0f, 700f, 1f, 0.5f)
    internal var dismissing = false
}

/** A niri workspace: a horizontal strip of columns with its own scroll position. */
class OvWorkspace(val id: Int, val columns: MutableList<OvColumn> = mutableListOf()) {
    /** Strip scroll, in unzoomed pixels. */
    internal val viewX = Spring(0f, 800f, 1f, 0.5f)
    internal val dy = Spring(0f, 800f, 1f, 0.5f)

    /** Index of the focused column; -1 until chosen. */
    internal var focus = -1
    internal val rect = RectF()
}
