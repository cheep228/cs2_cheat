package com.niri.recents.shared

import android.os.Bundle

/**
 * Everything the user can tune. Colours follow niri's defaults (focus ring #7fc8ff / #505050,
 * backdrop #262626); sizes are on-screen sizes in the overview, chosen for a phone.
 *
 * Shared by the settings app (which stores it) and the hook inside the launcher
 * (which reads it through [SettingsContract]).
 */
data class NiriSettings(
    val enabled: Boolean = true,
    /** Window size in the overview, as a fraction of the screen. */
    val zoom: Float = 0.34f,
    val gapsDp: Float = 12f,
    val workspaceGapDp: Float = 14f,
    val cornerDp: Float = 14f,
    val ringWidthDp: Float = 3f,
    val ringActive: Int = 0xFF7FC8FF.toInt(),
    val ringInactive: Int = 0xFF505050.toInt(),
    val backdrop: Int = 0xFF262626.toInt(),
    val workspaceColor: Int = 0xFF3A3A3A.toInt(),
    val wallpaper: Boolean = true,
    val shadows: Boolean = true,
    val labels: Boolean = false,
    val animSpeed: Float = 1f,
    /** Bumped by "reset workspaces"; the hook clears its saved layout when it changes. */
    val resetToken: Long = 0L,
    /** Material Design 3 look: system (Material You) colours, MD3 shapes and chips. */
    val md3: Boolean = false,
    /** New apps start a new workspace after this many windows; 0 = never (pure niri). */
    val perWorkspace: Int = 3,
    /** Tapping a window opens its workspace as live windows side by side (niri tiling). */
    val tiling: Boolean = false,
    /** Width of a new column in tiling mode, as a fraction of the screen. */
    val columnWidth: Float = 0.5f,
    /** Tapping a window opens its workspace as live windows tiled like Hyprland (dwindle). */
    val hypr: Boolean = false,
) {
    /** Some live-window mode is on. */
    val liveWindows get() = tiling || hypr

    fun toBundle(): Bundle = Bundle().apply {
        putBoolean(K_ENABLED, enabled)
        putFloat(K_ZOOM, zoom)
        putFloat(K_GAPS, gapsDp)
        putFloat(K_WS_GAP, workspaceGapDp)
        putFloat(K_CORNER, cornerDp)
        putFloat(K_RING, ringWidthDp)
        putInt(K_RING_ACTIVE, ringActive)
        putInt(K_RING_INACTIVE, ringInactive)
        putInt(K_BACKDROP, backdrop)
        putInt(K_WS_COLOR, workspaceColor)
        putBoolean(K_WALLPAPER, wallpaper)
        putBoolean(K_SHADOWS, shadows)
        putBoolean(K_LABELS, labels)
        putFloat(K_SPEED, animSpeed)
        putLong(K_RESET, resetToken)
        putBoolean(K_MD3, md3)
        putInt(K_PER_WS, perWorkspace)
        putBoolean(K_TILING, tiling)
        putFloat(K_COL_WIDTH, columnWidth)
        putBoolean(K_HYPR, hypr)
    }

    companion object {
        private const val K_ENABLED = "enabled"
        private const val K_ZOOM = "zoom"
        private const val K_GAPS = "gaps"
        private const val K_WS_GAP = "wsGap"
        private const val K_CORNER = "corner"
        private const val K_RING = "ring"
        private const val K_RING_ACTIVE = "ringActive"
        private const val K_RING_INACTIVE = "ringInactive"
        private const val K_BACKDROP = "backdrop"
        private const val K_WS_COLOR = "wsColor"
        private const val K_WALLPAPER = "wallpaper"
        private const val K_SHADOWS = "shadows"
        private const val K_LABELS = "labels"
        private const val K_SPEED = "speed"
        private const val K_RESET = "reset"
        private const val K_MD3 = "md3"
        private const val K_PER_WS = "perWs"
        private const val K_TILING = "tiling"
        private const val K_COL_WIDTH = "colWidth"
        private const val K_HYPR = "hypr"

        fun fromBundle(b: Bundle?): NiriSettings {
            val d = NiriSettings()
            if (b == null) return d
            return NiriSettings(
                enabled = b.getBoolean(K_ENABLED, d.enabled),
                zoom = b.getFloat(K_ZOOM, d.zoom),
                gapsDp = b.getFloat(K_GAPS, d.gapsDp),
                workspaceGapDp = b.getFloat(K_WS_GAP, d.workspaceGapDp),
                cornerDp = b.getFloat(K_CORNER, d.cornerDp),
                ringWidthDp = b.getFloat(K_RING, d.ringWidthDp),
                ringActive = b.getInt(K_RING_ACTIVE, d.ringActive),
                ringInactive = b.getInt(K_RING_INACTIVE, d.ringInactive),
                backdrop = b.getInt(K_BACKDROP, d.backdrop),
                workspaceColor = b.getInt(K_WS_COLOR, d.workspaceColor),
                wallpaper = b.getBoolean(K_WALLPAPER, d.wallpaper),
                shadows = b.getBoolean(K_SHADOWS, d.shadows),
                labels = b.getBoolean(K_LABELS, d.labels),
                animSpeed = b.getFloat(K_SPEED, d.animSpeed),
                resetToken = b.getLong(K_RESET, d.resetToken),
                md3 = b.getBoolean(K_MD3, d.md3),
                perWorkspace = b.getInt(K_PER_WS, d.perWorkspace),
                tiling = b.getBoolean(K_TILING, d.tiling),
                columnWidth = b.getFloat(K_COL_WIDTH, d.columnWidth),
                hypr = b.getBoolean(K_HYPR, d.hypr),
            )
        }
    }
}

/** How the launcher-side hook talks to this app. */
object SettingsContract {
    const val AUTHORITY = "com.niri.recents.settings"
    const val URI = "content://$AUTHORITY"
    const val METHOD_GET = "get"
    const val METHOD_REPORT = "report"

    /** In the "get" bundle: when the copied wallpaper last changed (0 = none). */
    const val KEY_WALLPAPER_STAMP = "wallpaperStamp"

    /** The real wallpaper, copied with root, served as a file. */
    const val WALLPAPER_URI = "$URI/wallpaper"

    /** From the launcher: the user pinched the overview to this window size (arg = zoom). */
    const val METHOD_SET_ZOOM = "setZoom"

    /** From the launcher: what the tiling desktop shows now (extras), plus its command binder. */
    const val METHOD_TILER_STATE = "tilerState"

    /** In the "get" extras from the launcher: its tiling command binder. */
    const val KEY_TILER_BINDER = "tilerBinder"
}

/** Tiling desktop protocol: commands the desktop sends to the launcher over a Binder. */
object TilerProtocol {
    const val DESCRIPTOR = "com.niri.recents.Tiler"

    /** int left, top, right, bottom: where windows may go (screen coordinates). */
    const val HELLO = 1
    const val FOCUS = 2 // int column
    const val SCROLL = 3 // int first visible column
    const val WIDTH = 4 // cycle the focused column's width
    const val CLOSE = 5 // close the focused app
    const val MOVE = 6 // int -1 / +1: move the focused column left / right
    const val MOVE_WS = 7 // int -1 / +1: move the focused column to the workspace above / below
    const val WORKSPACE = 8 // int -1 / +1: switch workspace
    const val OVERVIEW = 9 // open the niri overview
    const val FULLSCREEN = 10 // open the focused app normally, full screen
    const val HIDDEN = 11 // the desktop went out of sight
    const val SHOWN = 12 // the desktop is back
    const val WORKSPACE_TO = 13 // int index: go to that workspace (Waybar buttons)
    const val TOGGLE_SPLIT = 14 // Hyprland togglesplit on the focused window

    // State keys (launcher -> app).
    const val S_LABELS = "labels"
    const val S_PACKAGES = "packages"
    const val S_WIDTHS = "widths"
    const val S_FOCUS = "focus"
    const val S_FIRST = "first"
    const val S_LAST = "last"
    const val S_WS = "ws"
    const val S_WS_COUNT = "wsCount"
    const val S_MESSAGE = "message"
    const val S_ACTIVE = "active"

    /** "niri" or "hypr". */
    const val S_MODE = "mode"

    /** Screen bounds of the windows on screen, as left, top, right, bottom quadruples. */
    const val S_RECTS = "rects"

    /** Which of [S_RECTS] is the focused window (-1: none). */
    const val S_RECT_FOCUS = "rectFocus"

    const val MODE_NIRI = "niri"
    const val MODE_HYPR = "hypr"

    /** Waybar at the top of the Hyprland desktop, margins included (dp). */
    const val WAYBAR_DP = 46

    /** Column width presets, as in niri's preset-column-widths. */
    val WIDTHS = floatArrayOf(1f / 3f, 0.5f, 2f / 3f, 1f)

    /** Height of the desktop's control bar, margins included (dp); windows stay above it. */
    const val BAR_DP = 72
}
