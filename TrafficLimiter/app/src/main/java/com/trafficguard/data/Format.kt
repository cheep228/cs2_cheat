package com.trafficguard.data

import java.math.BigDecimal
import java.math.RoundingMode

const val MB = 1024L * 1024L

fun fmtBytes(b: Long): String {
    val v = b.toDouble()
    return when {
        b < 1024 -> "$b Б"
        b < MB -> "%.1f КБ".format(v / 1024)
        b < MB * 1024 -> "%.1f МБ".format(v / MB)
        else -> "%.2f ГБ".format(v / (MB * 1024))
    }
}

/** Bytes to a compact MB string for an input field; 0 becomes empty. */
fun bytesToMbText(b: Long): String {
    if (b <= 0) return ""
    return BigDecimal(b).divide(BigDecimal(MB), 3, RoundingMode.HALF_UP).stripTrailingZeros().toPlainString()
}

/** Parses "1,5" or "1.5" megabytes. Empty is 0. Null on garbage. */
fun mbTextToBytes(text: String): Long? {
    val t = text.trim().replace(',', '.')
    if (t.isEmpty()) return 0L
    val v = t.toDoubleOrNull() ?: return null
    if (v < 0 || v.isNaN() || v.isInfinite()) return null
    return (v * MB).toLong()
}

fun fmtDuration(ms: Long): String {
    val s = (ms + 999) / 1000
    return when {
        s < 60 -> "$s сек"
        s < 3600 -> "${(s + 59) / 60} мин"
        else -> "${s / 3600} ч ${(s % 3600) / 60} мин"
    }
}

fun quotaName(q: Quota) = when (q) {
    Quota.MINUTE -> "в минуту"
    Quota.HOUR -> "в час"
    Quota.DAY -> "в день"
    Quota.TOTAL -> "максимум"
}
