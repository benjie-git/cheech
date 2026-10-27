package com.octopilot.cheech.model

import android.content.Context
import kotlinx.serialization.encodeToString
import kotlinx.serialization.json.Json

class Prefs(context: Context) {
    private val prefs = context.getSharedPreferences("cheech", Context.MODE_PRIVATE)
    private val json = Json { ignoreUnknownKeys = true; encodeDefaults = true }

    var playerName: String
        get() = prefs.getString("playerName", "Player") ?: "Player"
        set(value) = prefs.edit().putString("playerName", value).apply()

    var playerColor: Int
        get() = prefs.getInt("playerColor", 1)
        set(value) = prefs.edit().putInt("playerColor", value.coerceIn(1, 8)).apply()

    var hostPort: Int
        get() = prefs.getInt("hostPort", 3838)
        set(value) = prefs.edit().putInt("hostPort", value.coerceIn(1, 65535)).apply()

    var joinHost: String
        get() = prefs.getString("joinHost", "127.0.0.1") ?: "127.0.0.1"
        set(value) = prefs.edit().putString("joinHost", value).apply()

    var joinPort: Int
        get() = prefs.getInt("joinPort", 3838)
        set(value) = prefs.edit().putInt("joinPort", value.coerceIn(1, 65535)).apply()

    var numPlayers: Int
        get() = prefs.getInt("numPlayers", 3)
        set(value) = prefs.edit().putInt("numPlayers", value.coerceIn(2, 6)).apply()

    var longJumps: Boolean
        get() = prefs.getBoolean("longJumps", true)
        set(value) = prefs.edit().putBoolean("longJumps", value).apply()

    var hopOthers: Boolean
        get() = prefs.getBoolean("hopOthers", true)
        set(value) = prefs.edit().putBoolean("hopOthers", value).apply()

    var stopOthers: Boolean
        get() = prefs.getBoolean("stopOthers", true)
        set(value) = prefs.edit().putBoolean("stopOthers", value).apply()

    var opponentStepMs: Int
        get() = prefs.getInt("opponentStepMs", 250).coerceIn(0, 500)
        set(value) = prefs.edit().putInt("opponentStepMs", value.coerceIn(0, 500)).apply()

    var lastBotType: String
        get() = prefs.getString("lastBotType", "LookAhead(3)") ?: "LookAhead(3)"
        set(value) = prefs.edit().putString("lastBotType", value).apply()

    var savedGame: String?
        get() = prefs.getString("savedGame", null)
        set(value) = prefs.edit().putString("savedGame", value).apply()

    fun loadSeats(): List<SeatConfig> {
        val raw = prefs.getString("seats", null) ?: return emptyList()
        return try {
            json.decodeFromString<List<SeatConfig>>(raw).takeIf { it.size in 2..6 } ?: emptyList()
        } catch (_: Exception) {
            emptyList()
        }
    }

    fun saveSeats(seats: List<SeatConfig>) {
        prefs.edit().putString("seats", json.encodeToString(seats)).apply()
    }
}
