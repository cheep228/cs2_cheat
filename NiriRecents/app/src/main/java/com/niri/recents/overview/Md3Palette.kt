package com.niri.recents.overview

import android.content.Context
import android.os.Build

/** The Material Design 3 dark-scheme roles the overview uses. */
class Md3Palette(
    val primary: Int,
    val primaryContainer: Int,
    val onPrimaryContainer: Int,
    val surface: Int,
    val surfaceContainer: Int,
    val surfaceContainerHigh: Int,
    val onSurface: Int,
    val onSurfaceVariant: Int,
    val outlineVariant: Int,
) {
    companion object {
        /** MD3 baseline dark scheme, for when the system has no dynamic colours. */
        val BASELINE = Md3Palette(
            primary = 0xFFD0BCFF.toInt(),
            primaryContainer = 0xFF4F378B.toInt(),
            onPrimaryContainer = 0xFFEADDFF.toInt(),
            surface = 0xFF141218.toInt(),
            surfaceContainer = 0xFF211F26.toInt(),
            surfaceContainerHigh = 0xFF2B2930.toInt(),
            onSurface = 0xFFE6E0E9.toInt(),
            onSurfaceVariant = 0xFFCAC4D0.toInt(),
            outlineVariant = 0xFF49454F.toInt(),
        )

        /** The system's Material You colours (from the wallpaper), dark scheme. */
        fun system(context: Context): Md3Palette = try {
            if (Build.VERSION.SDK_INT >= 34) {
                Md3Palette(
                    primary = context.getColor(android.R.color.system_primary_dark),
                    primaryContainer = context.getColor(android.R.color.system_primary_container_dark),
                    onPrimaryContainer = context.getColor(android.R.color.system_on_primary_container_dark),
                    surface = context.getColor(android.R.color.system_surface_dark),
                    surfaceContainer = context.getColor(android.R.color.system_surface_container_dark),
                    surfaceContainerHigh = context.getColor(android.R.color.system_surface_container_high_dark),
                    onSurface = context.getColor(android.R.color.system_on_surface_dark),
                    onSurfaceVariant = context.getColor(android.R.color.system_on_surface_variant_dark),
                    outlineVariant = context.getColor(android.R.color.system_outline_variant_dark),
                )
            } else {
                // Android 12-13: build the roles from the tonal palettes.
                val n1 = context.getColor(android.R.color.system_neutral1_900)
                val n1b = context.getColor(android.R.color.system_neutral1_800)
                Md3Palette(
                    primary = context.getColor(android.R.color.system_accent1_200),
                    primaryContainer = context.getColor(android.R.color.system_accent1_700),
                    onPrimaryContainer = context.getColor(android.R.color.system_accent1_100),
                    surface = n1,
                    surfaceContainer = blend(n1, n1b, 0.3f),
                    surfaceContainerHigh = blend(n1, n1b, 0.7f),
                    onSurface = context.getColor(android.R.color.system_neutral1_100),
                    onSurfaceVariant = context.getColor(android.R.color.system_neutral2_200),
                    outlineVariant = context.getColor(android.R.color.system_neutral2_700),
                )
            }
        } catch (t: Throwable) {
            BASELINE
        }

        private fun blend(a: Int, b: Int, t: Float): Int {
            fun ch(shift: Int) = (((a shr shift) and 0xFF) * (1 - t) + ((b shr shift) and 0xFF) * t).toInt()
            return (0xFF shl 24) or (ch(16) shl 16) or (ch(8) shl 8) or ch(0)
        }
    }
}
