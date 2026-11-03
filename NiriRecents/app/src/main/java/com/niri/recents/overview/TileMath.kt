package com.niri.recents.overview

import android.graphics.Rect

/** Window layouts of the live-window modes, as plain math (no Android state). */
object TileMath {

    /** Last column that fits in [width] when [first] is the first one shown. */
    fun lastFrom(px: List<Int>, first: Int, width: Int, gap: Int): Int {
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
    fun fitFirst(px: List<Int>, focus: Int, first: Int, width: Int, gap: Int): Int {
        if (px.isEmpty()) return 0
        var f = first.coerceIn(0, px.lastIndex)
        if (focus < f) f = focus
        while (f < focus && lastFrom(px, f, width, gap) < focus) f++
        var maxFirst = px.lastIndex
        while (maxFirst > 0 && lastFrom(px, maxFirst - 1, width, gap) == px.lastIndex) maxFirst--
        return minOf(f, maxFirst)
    }

    /** niri's scrolling strip: bounds of the columns on screen, by index. A short row is centred. */
    fun scrolling(px: List<Int>, first: Int, area: Rect, gap: Int): Map<Int, Rect> {
        if (px.isEmpty()) return emptyMap()
        val last = lastFrom(px, first, area.width(), gap)
        val total = (first..last).sumOf { px[it] } + (last - first) * gap
        var x = area.left + if (first == 0 && last == px.lastIndex) maxOf(0, (area.width() - total) / 2) else 0
        val out = LinkedHashMap<Int, Rect>()
        for (i in first..last) {
            out[i] = Rect(x, area.top, x + px[i], area.bottom)
            x += px[i] + gap
        }
        return out
    }

    /**
     * Hyprland's dwindle layout: every window splits the space left over by the one before it,
     * side by side when that space is wider than tall, one above the other otherwise
     * ([flipped] swaps that choice, like Hyprland's togglesplit). All [n] windows are on screen.
     * [widthMultiplier] is dwindle's split_width_multiplier: on a phone the halves are almost
     * square, and 1.3 keeps the familiar spiral (top, then side by side, then stacked...).
     */
    fun dwindle(
        n: Int,
        area: Rect,
        gap: Int,
        widthMultiplier: Float = 1.3f,
        flipped: (Int) -> Boolean = { false },
    ): List<Rect> {
        val out = ArrayList<Rect>(n)
        var r = Rect(area)
        for (i in 0 until n) {
            if (i == n - 1) {
                out += Rect(r)
                break
            }
            var sideBySide = r.width() * widthMultiplier > r.height()
            if (flipped(i)) sideBySide = !sideBySide
            if (sideBySide) {
                val mid = r.left + (r.width() - gap) / 2
                out += Rect(r.left, r.top, mid, r.bottom)
                r = Rect(mid + gap, r.top, r.right, r.bottom)
            } else {
                val mid = r.top + (r.height() - gap) / 2
                out += Rect(r.left, r.top, r.right, mid)
                r = Rect(r.left, mid + gap, r.right, r.bottom)
            }
        }
        return out
    }

    /** A column holding a split-screen pair gives each app an equal share, one above the other. */
    fun stack(r: Rect, n: Int, gap: Int): List<Rect> {
        if (n <= 1) return listOf(Rect(r))
        val each = (r.height() - gap * (n - 1)) / n
        return (0 until n).map { k ->
            val top = r.top + k * (each + gap)
            Rect(r.left, top, r.right, top + each)
        }
    }
}
