package app.djfusion.ui

import java.util.Locale

/** 3:07 */
fun formatTime(ms: Long): String {
    val total = (ms.coerceAtLeast(0)) / 1000
    return String.format(Locale.ROOT, "%d:%02d", total / 60, total % 60)
}

/** 3:07.4 */
fun formatTimeTenths(ms: Long): String {
    val m = ms.coerceAtLeast(0)
    val total = m / 1000
    return String.format(Locale.ROOT, "%d:%02d.%d", total / 60, total % 60, (m % 1000) / 100)
}

fun framesToMs(frames: Long): Long = frames * 1000L / 48_000L
