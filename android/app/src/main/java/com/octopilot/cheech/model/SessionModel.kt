package com.octopilot.cheech.model

import android.app.Application
import android.net.Uri
import android.os.SystemClock
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateListOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import androidx.lifecycle.AndroidViewModel
import com.octopilot.cheech.board.MoveAnimator
import com.octopilot.cheech.bridge.CheechSeatSpec
import com.octopilot.cheech.bridge.CheechSession
import com.octopilot.cheech.bridge.CheechSnapshot

enum class Screen { Setup, Game }
enum class SetupMode { Start, Join }
enum class GameAction { Leave, Restart, Rotate, Shuffle }

data class ChatMessage(val text: String, val system: Boolean = false)

class SessionModel(application: android.app.Application) :
    AndroidViewModel(application), CheechSession.Listener {

    private val prefs = Prefs(application)
    private val session = CheechSession(this)
    val animator = MoveAnimator()

    var screen by mutableStateOf(Screen.Setup)
        private set
    var setupMode by mutableStateOf(SetupMode.Start)

    var playerName by mutableStateOf(prefs.playerName)
    var playerColor by mutableStateOf(prefs.playerColor)
    var hostPort by mutableStateOf(prefs.hostPort)
    var joinHost by mutableStateOf(prefs.joinHost)
    var joinPort by mutableStateOf(prefs.joinPort)
    var joinSpectator by mutableStateOf(false)
    var numPlayers by mutableStateOf(prefs.numPlayers)
    var longJumps by mutableStateOf(prefs.longJumps)
    var hopOthers by mutableStateOf(prefs.hopOthers)
    var stopOthers by mutableStateOf(prefs.stopOthers)
    var opponentStepMs by mutableStateOf(prefs.opponentStepMs)
    var lastBotType by mutableStateOf(prefs.lastBotType)

    var seats by mutableStateOf(prefs.loadSeats())
        private set
    var snapshot by mutableStateOf<CheechSnapshot?>(null)
        private set

    var animatingPlayer by mutableStateOf(0)
        private set
    var messages = mutableStateListOf<ChatMessage>()
        private set
    var hasUnreadChat by mutableStateOf(false)
    var chatOpen by mutableStateOf(false)
    var fullScreen by mutableStateOf(false)
    var confirmAction by mutableStateOf<GameAction?>(null)
    var errorText by mutableStateOf<String?>(null)

    private var lastMoveSerial = 0
    private var lastTapHole = -1
    private var lastTapAt = 0L
    private var lastTapWasTerminal = false

    init {
        if (seats.size !in 2..6) {
            seats = emptyList()
            syncSeatCount(numPlayers)
        }
        resumeSavedGameIfNeeded()
    }

    // ---------------------------------------------------------------------
    // Setup

    fun syncSeatCount(count: Int) {
        val target = count.coerceIn(2, 6)
        val current = seats.toMutableList()
        while (current.size < target) {
            val i = current.size
            current.add(
                SeatConfig(
                    kind = SEAT_HUMAN,
                    color = (i % 8) + 1,
                    humanName = "Player ${i + 1}",
                )
            )
        }
        while (current.size > target) current.removeAt(current.lastIndex)
        for (i in current.indices) {
            if (current[i].kind == SEAT_HUMAN) {
                if (current[i].humanName.isNullOrBlank()) current[i] = current[i].copy(humanName = "Player ${i + 1}")
                if (current[i].color !in 1..8) current[i] = current[i].copy(color = (i % 8) + 1)
            }
        }
        seats = current
        saveSeats()
    }

    fun updateSeat(index: Int, transform: (SeatConfig) -> SeatConfig) {
        if (index !in seats.indices) return
        val list = seats.toMutableList()
        list[index] = transform(list[index])
        seats = list
        saveSeats()
    }

    fun setColor(index: Int, color: Int) {
        if (index !in seats.indices) return
        val list = seats.toMutableList()
        val other = list.indexOfFirst { it.color == color && it.kind != SEAT_REMOTE && it.id != list[index].id }
        if (other >= 0) list[other] = list[other].copy(color = list[index].color)
        list[index] = list[index].copy(color = color)
        seats = list
        saveSeats()
        if (list[index].kind == SEAT_HUMAN && list[index].humanName == playerName) {
            playerColor = color
            prefs.playerColor = color
        }
    }

    private fun saveSeats() = prefs.saveSeats(seats)

    fun updateProfile(name: String, color: Int) {
        val trimmed = name.trim().ifEmpty { "Player" }
        playerName = trimmed
        playerColor = color.coerceIn(1, 8)
        prefs.playerName = playerName
        prefs.playerColor = playerColor
        session.changeName(playerName)
        session.changeColor(playerColor)
        val index = seats.indexOfFirst { it.kind == SEAT_HUMAN && it.humanName == trimmed }
        if (index >= 0) updateSeat(index) { it.copy(color = color) }
    }

    fun startGame() {
        val count = numPlayers.coerceIn(2, 6)
        syncSeatCount(count)
        val specs = seats.mapIndexed { index, seat ->
            when (seat.kind) {
                SEAT_COMPUTER -> {
                    var type = seat.botType.ifEmpty { lastBotType }
                    val resolved = BotCatalog.resolvedType(type, seat.skill)
                    type = BotCatalog.withDepth(resolved.first, seat.depth)
                    val friendCount = seat.friendSeats?.size ?: (count - 1)
                    type = BotCatalog.cappedForSingleFriend(type, friendCount)
                    CheechSeatSpec(
                        kind = SEAT_COMPUTER,
                        botType = type,
                        name = seat.name.ifEmpty { BotCatalog.cuteName(type) },
                        color = seat.color,
                        smarts = resolved.second,
                        friendSeats = focusIndices(seat.friendSeats),
                        enemySeats = focusIndices(seat.enemySeats),
                    )
                }
                SEAT_REMOTE -> CheechSeatSpec(SEAT_REMOTE, "", "", 0, 100)
                else -> CheechSeatSpec(
                    kind = SEAT_HUMAN,
                    botType = "",
                    name = seat.humanName?.ifEmpty { "Player ${index + 1}" } ?: "Player ${index + 1}",
                    color = seat.color,
                    smarts = 100,
                )
            }
        }
        prefs.numPlayers = count
        prefs.longJumps = longJumps
        prefs.hopOthers = hopOthers
        prefs.stopOthers = stopOthers
        prefs.opponentStepMs = opponentStepMs
        prefs.hostPort = hostPort
        session.setAnimationStepMs(opponentStepMs)
        session.startGame(hostPort, count, longJumps, hopOthers, stopOthers, specs, WEB_PORT)
        resetTransients()
        screen = Screen.Game
    }

    fun joinGame() {
        val host = joinHost.trim().ifEmpty { "127.0.0.1" }
        if (joinPort !in 1..65535) {
            errorText = "Choose a server port between 1 and 65535."
            return
        }
        prefs.joinHost = host
        prefs.joinPort = joinPort
        prefs.playerName = playerName
        prefs.playerColor = playerColor
        session.setAnimationStepMs(opponentStepMs)
        session.joinHost(host, joinPort, joinSpectator, playerName, playerColor)
        resetTransients()
        screen = Screen.Game
    }

    fun handleInvite(uri: Uri) {
        val target = InviteLink.parse(uri.toString()) ?: return
        if (screen == Screen.Game) leave()
        joinHost = target.host
        joinPort = target.port
        setupMode = SetupMode.Join
        joinGame()
    }

    fun leave() {
        prefs.savedGame = null
        session.leave()
        resetTransients()
        fullScreen = false
        confirmAction = null
        errorText = null
        screen = Screen.Setup
    }

    fun requestLeave() {
        val s = snapshot
        if (s == null || !s.connected || s.isSpectator) {
            leave()
        } else {
            confirmAction = GameAction.Leave
        }
    }

    fun perform(action: GameAction) {
        when (action) {
            GameAction.Leave -> leave()
            GameAction.Restart -> session.restartGame()
            GameAction.Rotate -> session.rotatePlayers()
            GameAction.Shuffle -> session.shufflePlayers()
        }
        confirmAction = null
    }

    fun requestAction(action: GameAction) {
        val s = snapshot
        if (action == GameAction.Leave) {
            requestLeave()
            return
        }
        if (s == null || s.moveNumber == 0 || allPlayersFinished(s)) {
            perform(action)
        } else {
            confirmAction = action
        }
    }

    fun undoMove() = session.undoMove()

    fun reconfigureGame(players: Int, jumps: Boolean, hop: Boolean, stop: Boolean) {
        numPlayers = players.coerceIn(2, 6)
        longJumps = jumps
        hopOthers = hop
        stopOthers = stop
        prefs.numPlayers = numPlayers
        prefs.longJumps = longJumps
        prefs.hopOthers = hopOthers
        prefs.stopOthers = stopOthers
        syncSeatCount(numPlayers)
        session.reconfigureGame(numPlayers, longJumps, hopOthers, stopOthers)
    }

    fun addComputerPlayer(
        type: String,
        name: String,
        color: Int,
        friendPlayers: IntArray? = null,
        enemyPlayers: IntArray? = null,
    ) {
        lastBotType = type
        prefs.lastBotType = type
        session.addComputerPlayer(type, name.ifEmpty { BotCatalog.cuteName(type) }, color, friendPlayers, enemyPlayers)
    }

    fun removeComputerPlayers() = session.removeComputerPlayers()

    fun setWebPort(port: Int) = Unit

    fun persistLocalGame() {
        prefs.savedGame = session.localGameSave()
    }

    private fun resumeSavedGameIfNeeded() {
        val save = prefs.savedGame ?: return
        if (session.resumeLocalGame(save, WEB_PORT)) {
            resetTransients()
            screen = Screen.Game
        } else {
            prefs.savedGame = null
        }
    }

    // ---------------------------------------------------------------------
    // Snapshot / animation

    override fun onSnapshot(snapshot: CheechSnapshot) {
        val serial = snapshot.moveSerial
        this.snapshot = snapshot
        if (snapshot.status == 3) prefs.savedGame = null
        if (serial != lastMoveSerial) replayLatestMoveIfNeeded()
    }

    override fun onMessage(message: String) {
        messages.add(ChatMessage(message, message.startsWith("<<")))
        if (!chatOpen) hasUnreadChat = true
    }

    private fun replayLatestMoveIfNeeded() {
        val s = snapshot ?: return
        if (s.moveSerial == lastMoveSerial) return
        lastMoveSerial = s.moveSerial
        val skip = session.consumeReplaySkip()
        val path = s.lastMove
        if (path.size < 2) return
        val mover = s.playerAtHole(path.last())
        if (mover == s.myPlayerNumber || skip) return
        animatingPlayer = mover
        session.holdComputerMovesForAnimationHops(path.size - 1)
        val step = s.animationStepMs / 1000.0
        val done = if (isLocalHumanTurn(s)) 0f else s.animationDoneMs / 1000f
        animator.start(path.toList(), step, done) {
            animatingPlayer = 0
            replayLatestMoveIfNeeded()
        }
    }

    fun isLocalHumanTurn(s: CheechSnapshot? = snapshot): Boolean {
        val state = s ?: return false
        if (!state.connected) return false
        return if (state.isHost) {
            state.activeSeatKind == SEAT_HUMAN
        } else {
            !state.isSpectator && state.myPlayerNumber != 0 && state.currentPlayer == state.myPlayerNumber
        }
    }

    fun canControl(s: CheechSnapshot? = snapshot): Boolean {
        val state = s ?: return false
        return !state.isSpectator && (!state.isHost || state.activeSeatKind == SEAT_HUMAN)
    }

    fun allPlayersFinished(s: CheechSnapshot): Boolean =
        (1..s.numPlayers.coerceAtLeast(1)).all { s.finishedForPlayer(it) }

    fun winner(s: CheechSnapshot): Int {
        if (!allPlayersFinished(s)) return 0
        return (1..s.numPlayers.coerceAtLeast(1))
            .filter { s.finishedForPlayer(it) }
            .minByOrNull { s.finishedInMovesForPlayer(it) } ?: 0
    }

    fun displayedPlayer(s: CheechSnapshot): Int = if (animatingPlayer != 0) animatingPlayer else s.currentPlayer

    // ---------------------------------------------------------------------
    // Board interaction

    fun handleTap(hole: Int) {
        val s = snapshot ?: return
        if (!s.connected) return
        if (animator.isAnimating && !isLocalHumanTurn(s)) return
        val now = SystemClock.uptimeMillis()
        val selected = s.selected
        val isDouble = lastTapHole == hole && now - lastTapAt < 400
        if (isDouble && selected.size >= 2 && selected.last() == hole) {
            confirmSelected()
            return
        }
        if (isDouble && lastTapWasTerminal) {
            // The first tap of this double tap removed the final hole from
            // the path. Put it back before confirming the move.
            session.tapHole(hole)
            confirmSelected()
            return
        }
        lastTapHole = hole
        lastTapAt = now
        lastTapWasTerminal = selected.lastOrNull() == hole
        session.tapHole(hole)
    }

    fun confirmSelected() {
        lastTapHole = -1
        lastTapWasTerminal = false
        session.confirmMove()
    }

    fun clearSelection() {
        lastTapHole = -1
        lastTapWasTerminal = false
        session.clearSelection()
    }

    fun removeLastHop() {
        lastTapHole = -1
        lastTapWasTerminal = false
        session.removeLastHop()
    }

    fun sendChat(text: String) {
        val trimmed = text.trim()
        if (trimmed.isNotEmpty()) session.sendChat(trimmed)
    }

    fun markChatRead() {
        hasUnreadChat = false
    }

    private fun resetTransients() {
        lastMoveSerial = snapshot?.moveSerial ?: 0
        animator.stop()
        animatingPlayer = 0
        lastTapHole = -1
        lastTapAt = 0
        lastTapWasTerminal = false
        messages.clear()
        hasUnreadChat = false
        chatOpen = false
    }

    override fun onCleared() {
        persistLocalGame()
        session.close()
        super.onCleared()
    }

    private fun focusIndices(ids: List<String>?): IntArray? {
        if (ids == null) return null
        val result = seats.mapIndexedNotNull { index, seat ->
            if (seat.id in ids) index else null
        }
        return result.toIntArray()
    }

    companion object {
        const val WEB_PORT = 3839
    }
}
