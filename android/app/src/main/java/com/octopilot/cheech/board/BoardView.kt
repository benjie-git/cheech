package com.octopilot.cheech.board

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.drawscope.DrawScope
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.layout.onSizeChanged
import androidx.compose.ui.unit.toSize
import com.octopilot.cheech.model.SEAT_REMOTE
import com.octopilot.cheech.model.SessionModel
import kotlin.math.max
import kotlin.math.min

object PegColors {
    val colors = listOf(
        Color.Transparent,
        Color(0xFFCC2626),
        Color(0xFFF28C1A),
        Color(0xFFE6CC26),
        Color(0xFF339944),
        Color(0xFF2659CC),
        Color(0xFF8C33B3),
        Color(0xFF262626),
        Color(0xFFE6E6E6),
    )
    val names = listOf("", "Red", "Orange", "Yellow", "Green", "Blue", "Purple", "Black", "White")

    fun of(index: Int): Color = colors.getOrElse(index) { Color.Gray }
}

@Composable
fun BoardView(model: SessionModel, modifier: Modifier = Modifier) {
    val snapshot = model.snapshot
    var size by remember { mutableStateOf(Size.Zero) }

    LaunchedEffect(model.animator.active?.token) {
        while (model.animator.active != null) {
            androidx.compose.runtime.withFrameNanos { model.animator.tick(it) }
        }
    }

    if (snapshot == null) {
        Canvas(modifier.onSizeChanged { size = it.toSize() }) {}
        return
    }

    val players = snapshot.numPlayers.coerceIn(2, 6)
    val localPlayer = when {
        snapshot.myPlayerNumber in 1..6 -> snapshot.myPlayerNumber
        snapshot.localHumanPlayerNumber in 1..6 -> snapshot.localHumanPlayerNumber
        else -> 1
    }
    val rotation = BoardGeometry.rotation(players, localPlayer)
    val layout = remember(size, rotation, players) {
        BoardGeometry.layout(size, rotation)
    }
    val pointsList = remember(layout) { layout.points.toList() }

    Canvas(
        modifier = modifier
            .onSizeChanged { size = it.toSize() }
            .pointerInput(layout) {
                detectTapGestures { offset ->
                    val threshold = max(26f, layout.unit * 0.9f)
                    val hole = BoardGeometry.nearestHole(layout, offset, threshold)
                    if (hole != null) model.handleTap(hole)
                }
            }
    ) {
        if (size.width <= 0f || size.height <= 0f) return@Canvas

        val boardPath = BoardGeometry.hexBoardPath(layout)
        drawPath(boardPath, Color(0xFF8C663D))
        drawPath(boardPath, Color(0xFF6B4A2B), style = Stroke(width = max(2f, layout.unit * 0.1f)))

        // Lattice
        val lattice = Color(0xFF7D5733)
        for (i in 0 until BoardGeometry.COUNT) {
            if (!BoardGeometry.present[i]) continue
            val row = i / BoardGeometry.SIZE_X
            val forward = if (row % 2 == 0) intArrayOf(i + 1, i + 14, i + 13)
            else intArrayOf(i + 1, i + 13, i + 12)
            for (j in forward) {
                if (j in 0 until BoardGeometry.COUNT && BoardGeometry.present[j]) {
                    drawLine(lattice, layout.points[i], layout.points[j], strokeWidth = max(1f, layout.unit * 0.035f))
                }
            }
        }

        val holeRadius = max(2f, layout.unit * 0.46f - 2f)
        for (i in 0 until BoardGeometry.COUNT) {
            if (!BoardGeometry.present[i]) continue
            drawCircle(Color(0xFF664228), radius = holeRadius, center = layout.points[i])
            drawCircle(Color(0xFF4A2E18), radius = holeRadius, center = layout.points[i], style = Stroke(width = max(1f, layout.unit * 0.03f)))
        }

        val showPegs = snapshot.connected &&
            (snapshot.status != 0 || snapshot.hasRemoteSeat || !snapshot.isHost)
        if (showPegs) {
            val animatingPath = model.animator.active?.path
            val movingTo = if (animatingPath != null && animatingPath.isNotEmpty()) {
                animatingPath.last()
            } else {
                -1
            }
            for (i in 0 until BoardGeometry.COUNT) {
                if (i == movingTo) continue
                val owner = snapshot.playerAtHole(i)
                if (owner <= 0) continue
                val color = snapshot.colorForPlayer(owner)
                if (color <= 0) continue
                drawPeg(layout.points[i], PegColors.of(color), layout.unit)
            }
            val moving = model.animator.position(pointsList)
            if (moving != null && movingTo >= 0) {
                val owner = snapshot.playerAtHole(movingTo)
                val color = if (owner > 0) snapshot.colorForPlayer(owner) else 0
                if (color > 0) {
                    drawPeg(moving, PegColors.of(color), layout.unit)
                }
            }
        }

        drawSelection(layout, snapshot.selected, Color(0xFFFFD633), layout.unit)
        drawSelection(layout, snapshot.telegraph, Color(0xFFFFD633).copy(alpha = 0.55f), layout.unit)

    }
}

private fun DrawScope.drawPeg(center: Offset, color: Color, unit: Float) {
    val radius = max(3f, unit * 0.36f)
    drawCircle(Color.Black.copy(alpha = 0.18f), radius = radius * 1.05f, center = center + Offset(0f, radius * 0.12f))
    drawCircle(color, radius = radius, center = center)
    drawCircle(Color.White.copy(alpha = 0.75f), radius = radius, center = center, style = Stroke(width = max(1f, unit * 0.035f)))
    drawCircle(Color.White.copy(alpha = 0.4f), radius = radius * 0.28f, center = center - Offset(radius * 0.25f, radius * 0.3f))
}

private fun DrawScope.drawSelection(layout: BoardGeometry.BoardLayout, holes: IntArray, color: Color, unit: Float) {
    if (holes.isEmpty()) return
    val path = Path()
    holes.forEachIndexed { index, hole ->
        if (hole !in 0 until BoardGeometry.COUNT) return@forEachIndexed
        val point = layout.points[hole]
        if (index == 0) path.moveTo(point.x, point.y) else path.lineTo(point.x, point.y)
    }
    if (holes.size > 1) {
        drawPath(path, color, style = Stroke(width = max(2f, unit * 0.14f)))
    }
    holes.forEach { hole ->
        if (hole in 0 until BoardGeometry.COUNT) {
            drawCircle(color.copy(alpha = 0.9f), radius = max(3f, unit * 0.42f), center = layout.points[hole], style = Stroke(width = max(2f, unit * 0.1f)))
        }
    }
}
