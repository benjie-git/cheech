package com.octopilot.cheech.model

import kotlinx.serialization.encodeToString
import kotlinx.serialization.json.Json
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Test

class BotCatalogTest {
    @Test
    fun withDepthReplacesExistingDepth() {
        assertEquals("LookAhead(5)", BotCatalog.withDepth("LookAhead(3)", 5))
        assertEquals("Friendly(2)", BotCatalog.withDepth("Friendly(4)", 2))
    }

    @Test
    fun withNullDepthKeepsType() {
        assertEquals("Mean(4)", BotCatalog.withDepth("Mean(4)", null))
    }

    @Test
    fun friendlyCapsDepthWithOneFriend() {
        assertEquals("Friendly(3)", BotCatalog.cappedForSingleFriend("Friendly(4)", 1))
        assertEquals("Friendly(4)", BotCatalog.cappedForSingleFriend("Friendly(4)", 0))
        assertEquals("Friendly(4)", BotCatalog.cappedForSingleFriend("Friendly(4)", 2))
        assertEquals("LookAhead(4)", BotCatalog.cappedForSingleFriend("LookAhead(4)", 1))
    }

    @Test
    fun lookAheadSkillMappingMatchesIos() {
        assertEquals("LookAhead(3)" to 100, BotCatalog.resolvedType("LookAhead(3)", 1))
        assertEquals("LookAhead(3)" to 90, BotCatalog.resolvedType("LookAhead(3)", 2))
        assertEquals("LookAhead(2)" to 100, BotCatalog.resolvedType("LookAhead(3)", 3))
        assertEquals("LookAhead(2)" to 80, BotCatalog.resolvedType("LookAhead(3)", 4))
    }

    @Test
    fun adversarialSkillMappingPreservesDepth() {
        assertEquals("Mean(4)" to 100, BotCatalog.resolvedType("Mean(4)", 1))
        assertEquals("Mean(4)" to 70, BotCatalog.resolvedType("Mean(4)", 4))
        assertEquals("Friendly(3)" to 90, BotCatalog.resolvedType("Friendly(3)", 2))
    }

    @Test
    fun skillIsClamped() {
        assertEquals("Mean(4)" to 100, BotCatalog.resolvedType("Mean(4)", 0))
        assertEquals("Mean(4)" to 70, BotCatalog.resolvedType("Mean(4)", 99))
        assertEquals(BotSkill.BEST, BotSkill.fromLevel(0))
        assertEquals(BotSkill.NERFED, BotSkill.fromLevel(4))
    }
}

class InviteLinkTest {
    @Test
    fun parsesJoinLink() {
        assertEquals(
            InviteTarget("192.168.1.20", 3838),
            InviteLink.parse("cheech://join?host=192.168.1.20&port=3838"),
        )
    }

    @Test
    fun appliesDefaultPort() {
        assertEquals(InviteTarget("host.local", 3838), InviteLink.parse("cheech://host.local"))
    }

    @Test
    fun parsesHostPortForm() {
        assertEquals(InviteTarget("10.0.0.5", 4000), InviteLink.parse("cheech://10.0.0.5:4000"))
    }

    @Test
    fun parsesEncodedQuery() {
        assertEquals(
            InviteTarget("my-host.local", 3838),
            InviteLink.parse("cheech://join?host=my-host.local&port=3838"),
        )
    }

    @Test
    fun rejectsInvalidInput() {
        assertNull(InviteLink.parse("https://example.com"))
        assertNull(InviteLink.parse("cheech://join"))
        assertNull(InviteLink.parse("cheech://join?host="))
        assertNull(InviteLink.parse("not a uri"))
    }
}

class SeatConfigTest {
    private val json = Json { ignoreUnknownKeys = true; encodeDefaults = true }

    @Test
    fun roundTripsSeatConfiguration() {
        val original = SeatConfig(
            id = "seat-1",
            kind = SEAT_COMPUTER,
            botType = "Mean(4)",
            name = "Robo",
            color = 3,
            skill = 2,
            depth = 4,
            friendSeats = listOf("seat-2"),
            enemySeats = emptyList(),
        )
        val decoded = json.decodeFromString<SeatConfig>(json.encodeToString(original))
        assertEquals(original, decoded)
        assertEquals(emptyList<String>(), decoded.enemySeats)
    }

    @Test
    fun nullFocusListsMeanEveryone() {
        val original = SeatConfig(id = "seat-1", friendSeats = null, enemySeats = null)
        val decoded = json.decodeFromString<SeatConfig>(json.encodeToString(original))
        assertNull(decoded.friendSeats)
        assertNull(decoded.enemySeats)
    }
}
