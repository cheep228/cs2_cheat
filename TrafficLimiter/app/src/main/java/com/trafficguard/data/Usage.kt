package com.trafficguard.data

import org.json.JSONObject
import java.util.TimeZone

/** Calendar windows in local time, as plain numbers so they compare cheaply. */
object TimeKeys {
    const val MINUTE = 60_000L
    const val HOUR = 3_600_000L
    const val DAY = 86_400_000L

    fun local(now: Long): Long = now + TimeZone.getDefault().getOffset(now)
    fun minute(now: Long) = Math.floorDiv(local(now), MINUTE)
    fun hour(now: Long) = Math.floorDiv(local(now), HOUR)
    fun day(now: Long) = Math.floorDiv(local(now), DAY)

    fun untilNext(window: Long, now: Long): Long = window - Math.floorMod(local(now), window)

    /** Start of the current local day, as epoch millis. */
    fun startOfDay(now: Long): Long = now - Math.floorMod(local(now), DAY)
}

/**
 * Traffic counted for one profile. Each window carries the key it belongs to;
 * when the clock moves into a new window that counter starts over at zero.
 */
data class Usage(
    val minuteKey: Long = -1,
    val minute: Long = 0,
    val hourKey: Long = -1,
    val hour: Long = 0,
    val dayKey: Long = -1,
    val day: Long = 0,
    val total: Long = 0,
    /** When the total was last reset. */
    val totalSince: Long = 0,
) {
    fun rolled(now: Long): Usage {
        val m = TimeKeys.minute(now)
        val h = TimeKeys.hour(now)
        val d = TimeKeys.day(now)
        return copy(
            minuteKey = m, minute = if (m == minuteKey) minute else 0,
            hourKey = h, hour = if (h == hourKey) hour else 0,
            dayKey = d, day = if (d == dayKey) day else 0,
        )
    }

    fun plus(bytes: Long, now: Long): Usage {
        val r = rolled(now)
        if (bytes <= 0) return r
        return r.copy(
            minute = r.minute + bytes,
            hour = r.hour + bytes,
            day = r.day + bytes,
            total = r.total + bytes,
        )
    }

    fun toJson(): String = JSONObject()
        .put("mk", minuteKey).put("m", minute)
        .put("hk", hourKey).put("h", hour)
        .put("dk", dayKey).put("d", day)
        .put("t", total).put("ts", totalSince)
        .toString()

    companion object {
        fun fromJson(text: String): Usage {
            val o = JSONObject(text)
            return Usage(
                o.optLong("mk", -1), o.optLong("m"),
                o.optLong("hk", -1), o.optLong("h"),
                o.optLong("dk", -1), o.optLong("d"),
                o.optLong("t"), o.optLong("ts"),
            )
        }
    }
}
