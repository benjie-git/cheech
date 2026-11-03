package com.octopilot.cheech.model

import androidx.annotation.Keep
import java.util.UUID
import kotlinx.serialization.Serializable

const val SEAT_HUMAN = 0
const val SEAT_COMPUTER = 1
const val SEAT_REMOTE = 2

@Keep
@Serializable
data class SeatConfig(
    val id: String = UUID.randomUUID().toString(),
    var kind: Int = SEAT_HUMAN,
    var botType: String = "LookAhead(3)",
    var name: String = "",
    var color: Int = 1,
    var humanName: String? = null,
    var skill: Int = 1,
    var depth: Int? = null,
    var friendSeats: List<String>? = null,
    var enemySeats: List<String>? = null,
)

enum class BotSkill(val level: Int, val label: String, val detail: String) {
    BEST(1, "Best", "Best move"),
    GREAT(2, "Great", "Skips ~10% of steps"),
    MID(3, "Mid", "Skips ~20% of steps"),
    NERFED(4, "Nerfed", "Skips ~40% of steps");

    companion object {
        fun fromLevel(level: Int): BotSkill = entries.firstOrNull { it.level == level } ?: BEST
    }
}

object BotCatalog {
    val types = listOf("Friendly(4)", "LookAhead(3)", "Mean(4)")

    fun cuteName(type: String): String {
        val name = com.octopilot.cheech.bridge.CheechSession.defaultNameForComputerType(type)
        return name.ifEmpty { "Computer" }
    }

    fun familyName(type: String): String {
        val name = com.octopilot.cheech.bridge.CheechSession.typeNameForComputerType(type)
        return name.ifEmpty { type.substringBefore('(') }
    }

    fun withDepth(type: String, depth: Int?): String {
        if (depth == null) return type
        val base = type.substringBefore('(')
        return "$base($depth)"
    }

    fun cappedForSingleFriend(type: String, friendCount: Int): String {
        if (!type.startsWith("Friendly")) return type
        if (friendCount != 1) return type
        val depth = type.substringAfter('(', "4").substringBefore(')').toIntOrNull() ?: 4
        return if (depth > 3) withDepth(type, 3) else type
    }

    fun resolvedType(type: String, skill: Int): Pair<String, Double> {
        val level = skill.coerceIn(1, 4)
        // Mirrors the de-skilling ladder in src/bot_deskilling.hh.
        val skipChance = doubleArrayOf(0.0, 0.10, 0.20, 0.40)[level - 1]
        val base = type.substringBefore('(')
        val depth = type.substringAfter('(', "4").substringBefore(')').toIntOrNull() ?: 4
        return when (base) {
            "LookAhead" -> "LookAhead(3)" to skipChance
            "Mean" -> "Mean($depth)" to skipChance
            "Friendly" -> "Friendly($depth)" to skipChance
            else -> type to 0.0
        }
    }
}
