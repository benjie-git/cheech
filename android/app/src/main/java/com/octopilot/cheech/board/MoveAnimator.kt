package com.octopilot.cheech.board

import android.os.SystemClock
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import androidx.compose.ui.geometry.Offset
import kotlin.math.floor

class MoveAnimator {
    data class ActiveMove(
        val path: IntArray,
        val startMs: Long,
        val stepSec: Double,
        val doneSec: Double,
        val token: Int,
    )

    var active by mutableStateOf<ActiveMove?>(null)
        private set

    var nowMs by mutableStateOf(0L)
        private set

    private var nextToken = 1
    private var completion: (() -> Unit)? = null

    val isAnimating: Boolean get() = active != null

    fun start(path: List<Int>, stepSeconds: Double, doneSeconds: Float, onDone: (() -> Unit)? = null) {
        if (path.size < 2) return
        nextToken++
        val start = SystemClock.uptimeMillis()
        active = ActiveMove(
            path = path.toIntArray(),
            startMs = start,
            stepSec = stepSeconds,
            doneSec = doneSeconds.toDouble(),
            token = nextToken,
        )
        nowMs = start
        completion = onDone
    }

    fun start(path: IntArray, stepSeconds: Double, doneSeconds: Float, onDone: (() -> Unit)? = null) {
        start(path.toList(), stepSeconds, doneSeconds, onDone)
    }

    fun stop() {
        nextToken++
        active = null
        completion = null
    }

    fun tick(frameTimeNanos: Long) {
        val move = active ?: return
        val elapsed = frameTimeNanos / 1_000_000L
        nowMs = elapsed
        val total = move.stepSec * (move.path.size - 1) + move.doneSec
        if (elapsed - move.startMs >= total * 1000.0) {
            active = null
            val callback = completion
            completion = null
            callback?.invoke()
        }
    }

    fun position(points: List<Offset>, atMs: Long? = null): Offset? {
        val move = active ?: return null
        if (points.size < move.path.size) return null
        val elapsedSec = (((atMs ?: nowMs) - move.startMs).coerceAtLeast(0L)) / 1000.0
        if (move.stepSec <= 0.0) {
            return points[move.path.last()]
        }
        val raw = elapsedSec / move.stepSec
        val hop = floor(raw).toInt().coerceIn(0, move.path.size - 2)
        val local = (raw - hop).coerceIn(0.0, 1.0)
        val eased = if (local < 0.5) 2.0 * local * local else 1.0 - 2.0 * (1.0 - local) * (1.0 - local)
        val from = points[move.path[hop]]
        val to = points[move.path[hop + 1]]
        return androidx.compose.ui.geometry.lerp(from, to, eased.toFloat())
    }
}
