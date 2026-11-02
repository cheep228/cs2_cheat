package com.niri.recents.shared

import android.os.Bundle

/**
 * Everything the user can tune. Defaults follow niri's own defaults where niri has one:
 * zoom 0.5, gaps 16, focus ring 4 with #7fc8ff / #505050, backdrop #262626.
 *
 * Shared by the settings app (which stores it) and the hook inside the launcher
 * (which reads it through [SettingsContract]).
 */
data class NiriSettings(
    val enabled: Boolean = true,
    val zoom: Float = 0.5f,
    val gapsDp: Float = 16f,
    val workspaceGapDp: Float = 28f,
    val cornerDp: Float = 18f,
    val ringWidthDp: Float = 4f,
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
) {
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
}
