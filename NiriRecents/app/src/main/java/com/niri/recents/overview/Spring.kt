package com.niri.recents.overview

import kotlin.math.abs
import kotlin.math.min
import kotlin.math.sqrt

/**
 * A damped spring with unit mass: x'' = -k (x - target) - c x', c = 2 ζ √k.
 * niri animates almost everything with these (damping-ratio 1.0, stiffness 800-1000),
 * which is what gives it the quick, non-bouncy feel.
 */
class Spring(
    initial: Float,
    var stiffness: Float,
    var dampingRatio: Float = 1f,
    /** Rest threshold, in the units of the value. */
    var epsilon: Float,
) {
    var value: Float = initial
        private set
    var target: Float = initial
        private set
    var velocity: Float = 0f
        private set

    val isAnimating: Boolean
        get() = abs(value - target) > epsilon || abs(velocity) > epsilon * 10f

    fun animateTo(newTarget: Float, initialVelocity: Float = velocity) {
        target = newTarget
        velocity = initialVelocity
    }

    fun snapTo(v: Float) {
        value = v
        target = v
        velocity = 0f
    }

    /** Jumps to the end of the current animation. */
    fun finish() = snapTo(target)

    fun isNear(tolerance: Float) = abs(value - target) <= tolerance

    /** Advances by [dt] seconds. Returns true while still moving. */
    fun step(dt: Float): Boolean {
        if (!isAnimating) {
            value = target
            velocity = 0f
            return false
        }
        val damping = 2f * dampingRatio * sqrt(stiffness)
        var left = dt
        while (left > 0f) {
            val h = min(left, 0.002f)
            val accel = -stiffness * (value - target) - damping * velocity
            velocity += accel * h
            value += velocity * h
            left -= h
        }
        if (!isAnimating) {
            value = target
            velocity = 0f
            return false
        }
        return true
    }
}
