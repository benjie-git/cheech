package com.octopilot.cheech.board

import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Path
import com.octopilot.cheech.bridge.CheechSession
import kotlin.math.PI
import kotlin.math.acos
import kotlin.math.cos
import kotlin.math.hypot
import kotlin.math.min
import kotlin.math.sin
import kotlin.math.sqrt
import kotlin.math.tan

object BoardGeometry {
    const val SIZE_X = 13
    const val COUNT = 247
    private val startMap = arrayOf(
        intArrayOf(0, 0, 0, 0, 0, 0, 0),
        intArrayOf(0, 1, 0, 0, 0, 0, 0),
        intArrayOf(0, 1, 0, 0, 2, 0, 0),
        intArrayOf(0, 1, 0, 2, 0, 3, 0),
        intArrayOf(0, 0, 1, 2, 0, 3, 4),
        intArrayOf(0, 0, 1, 2, 3, 4, 5),
        intArrayOf(0, 1, 2, 3, 4, 5, 6),
    )
    private val boardMap = intArrayOf(
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, 14, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, 14, 14, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, 14, 14, 14, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, 14, 14, 14, 14, -1, -1, -1, -1, -1,
        63, 63, 63, 63, 0, 0, 0, 0, 0, 25, 25, 25, 25,
        63, 63, 63, 0, 0, 0, 0, 0, 0, 25, 25, 25, -1,
        -1, 63, 63, 0, 0, 0, 0, 0, 0, 0, 25, 25, -1,
        -1, 63, 0, 0, 0, 0, 0, 0, 0, 0, 25, -1, -1,
        -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1,
        -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1,
        -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1,
        -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1,
        -1, -1, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1,
        -1, -1, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1,
        -1, -1, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, 0, 0, 0, 0, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, 0, 0, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1,
    )

    val present: BooleanArray by lazy { CheechSession.boardHolePresent() }
    val baseX: DoubleArray by lazy { CheechSession.boardHoleX() }
    val baseY: DoubleArray by lazy { CheechSession.boardHoleY() }
    val base: Array<Offset> by lazy {
        Array(COUNT) { Offset(baseX[it].toFloat(), baseY[it].toFloat()) }
    }

    fun startHoles(players: Int, player: Int): IntArray {
        val row = startMap[players.coerceIn(2, 6)]
        return (0 until COUNT).filter { i ->
            val code = boardMap.getOrElse(i) { -1 }
            code >= 0 && row[code % 10] == player
        }.toIntArray()
    }

    fun goalHoles(players: Int, player: Int): IntArray {
        val row = startMap[players.coerceIn(2, 6)]
        return (0 until COUNT).filter { i ->
            val code = boardMap.getOrElse(i) { -1 }
            code >= 0 && row[code / 10] == player
        }.toIntArray()
    }

    fun rotation(players: Int, player: Int): Float = ((1 - player) * PI / 3.0).toFloat()

    fun layout(size: Size, rotation: Float): BoardLayout {
        val cx = 6.5f
        val cy = 6f * sqrt(3f) / 2f
        val cosR = cos(rotation)
        val sinR = sin(rotation)
        val transformed = Array(COUNT) { i ->
            val p = base[i] - Offset(cx, cy)
            Offset(
                cx + p.x * cosR - p.y * sinR,
                cy + p.x * sinR + p.y * cosR,
            )
        }
        var minX = Float.MAX_VALUE
        var maxX = -Float.MAX_VALUE
        var minY = Float.MAX_VALUE
        var maxY = -Float.MAX_VALUE
        for (i in 0 until COUNT) {
            if (!present[i]) continue
            val p = transformed[i]
            minX = minOf(minX, p.x)
            maxX = maxOf(maxX, p.x)
            minY = minOf(minY, p.y)
            maxY = maxOf(maxY, p.y)
        }
        val boardW = (maxX - minX).coerceAtLeast(0.001f)
        val boardH = (maxY - minY).coerceAtLeast(0.001f)
        val scale = minOf(size.width / boardW, size.height / boardH) * 0.80f
        val offsetX = (size.width - boardW * scale) / 2f
        val offsetY = (size.height - boardH * scale) / 2f
        val points = Array(COUNT) { i ->
            val p = transformed[i]
            Offset(
                (p.x - minX) * scale + offsetX,
                (p.y - minY) * scale + offsetY,
            )
        }
        return BoardLayout(size, scale, rotation, points)
    }

    fun nearestHole(layout: BoardLayout, point: Offset, maxDistance: Float): Int? {
        var best = -1
        var bestDistance = maxDistance
        for (i in 0 until COUNT) {
            if (!present[i]) continue
            val distance = (layout.points[i] - point).getDistance()
            if (distance <= bestDistance) {
                bestDistance = distance
                best = i
            }
        }
        return if (best >= 0) best else null
    }

    fun hexBoardPath(layout: BoardLayout): Path {
        // The holes form a stair-stepped hexagon, so their raw convex hull has
        // nine or more vertices.  Build a true six-sided hull from six half
        // planes instead; every hole is guaranteed to be inside it.
        var minX = Float.MAX_VALUE
        var maxX = -Float.MAX_VALUE
        var minY = Float.MAX_VALUE
        var maxY = -Float.MAX_VALUE
        for (i in 0 until COUNT) {
            if (!present[i]) continue
            val p = layout.points[i]
            minX = minOf(minX, p.x)
            maxX = maxOf(maxX, p.x)
            minY = minOf(minY, p.y)
            maxY = maxOf(maxY, p.y)
        }
        val cx = (minX + maxX) * 0.5f
        val cy = (minY + maxY) * 0.5f
        val padding = layout.unit * 1.2f

        val normals = Array(6) { i ->
            val angle = layout.rotation + i * PI.toFloat() / 3f
            Offset(cos(angle), sin(angle))
        }
        val distances = FloatArray(6) { i ->
            val n = normals[i]
            var farthest = 0f
            for (hole in 0 until COUNT) {
                if (!present[hole]) continue
                val p = layout.points[hole] - Offset(cx, cy)
                farthest = maxOf(farthest, p.x * n.x + p.y * n.y)
            }
            farthest + padding
        }
        val vertices = Array(6) { i ->
            val n1 = normals[i]
            val n2 = normals[(i + 1) % 6]
            val d1 = distances[i]
            val d2 = distances[(i + 1) % 6]
            val perpendicular = Offset(-n1.y, n1.x)
            val denominator = n2.x * perpendicular.x + n2.y * perpendicular.y
            val t = if (kotlin.math.abs(denominator) < 0.0001f) {
                0f
            } else {
                (d2 - d1 * (n1.x * n2.x + n1.y * n2.y)) / denominator
            }
            Offset(
                cx + n1.x * d1 + perpendicular.x * t,
                cy + n1.y * d1 + perpendicular.y * t,
            )
        }
        return roundedPolygonPath(vertices.toList(), layout.unit * 0.8f)
    }

    private fun roundedPolygonPath(vertices: List<Offset>, cornerRadius: Float): Path {
        val path = Path()
        if (vertices.size <= 2) return path
        if (cornerRadius <= 0f) {
            path.moveTo(vertices[0].x, vertices[0].y)
            for (i in 1 until vertices.size) path.lineTo(vertices[i].x, vertices[i].y)
            path.close()
            return path
        }

        val entries = Array(vertices.size) { Offset.Zero }
        val exits = Array(vertices.size) { Offset.Zero }
        for (i in vertices.indices) {
            val prev = vertices[(i + vertices.size - 1) % vertices.size]
            val curr = vertices[i]
            val next = vertices[(i + 1) % vertices.size]
            val toPrev = unitVector(prev - curr)
            val toNext = unitVector(next - curr)
            val dot = (toPrev.x * toNext.x + toPrev.y * toNext.y).coerceIn(-1f, 1f)
            val angle = acos(dot)
            val maxTangent = min(hypot(curr.x - prev.x, curr.y - prev.y),
                hypot(curr.x - next.x, curr.y - next.y)) * 0.5f
            val tangent = min(cornerRadius / tan(angle * 0.5f), maxTangent)
            entries[i] = curr + toPrev * tangent
            exits[i] = curr + toNext * tangent
        }

        path.moveTo(entries[0].x, entries[0].y)
        path.quadraticTo(vertices[0].x, vertices[0].y, exits[0].x, exits[0].y)
        for (i in 1 until vertices.size) {
            path.lineTo(entries[i].x, entries[i].y)
            path.quadraticTo(vertices[i].x, vertices[i].y, exits[i].x, exits[i].y)
        }
        path.lineTo(entries[0].x, entries[0].y)
        path.close()
        return path
    }

    private fun unitVector(v: Offset): Offset {
        val length = v.getDistance()
        return if (length > 0f) v / length else Offset.Zero
    }

    data class BoardLayout(
        val size: Size,
        val unit: Float,
        val rotation: Float,
        val points: Array<Offset>,
    )
}
