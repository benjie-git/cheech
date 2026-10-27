package com.octopilot.cheech.bridge

import androidx.annotation.Keep

@Keep
interface CheechListener {
    fun onSnapshot(snapshot: CheechSnapshot)
    fun onMessage(message: String)
}

@Keep
class CheechSeatSpec(
    @JvmField val kind: Int,
    @JvmField val botType: String,
    @JvmField val name: String,
    @JvmField val color: Int,
    @JvmField val smarts: Int,
    @JvmField val friendSeats: IntArray? = null,
    @JvmField val enemySeats: IntArray? = null,
)

/**
 * A single packed snapshot built by the native session.  Keeping it to one
 * JNI crossing matters while moves are being replayed.
 */
@Keep
class CheechSnapshot(
    @JvmField val ints: IntArray,
    @JvmField val names: Array<String>,
    @JvmField val serverHost: String,
) {
    val connected get() = ints[CONNECTED] != 0
    val isHost get() = ints[IS_HOST] != 0
    val isSpectator get() = ints[IS_SPECTATOR] != 0
    val activeSeatKind get() = ints[ACTIVE_SEAT_KIND]
    val hasLocalHumanSeat get() = ints[HAS_LOCAL_HUMAN_SEAT] != 0
    val hasRemoteSeat get() = ints[HAS_REMOTE_SEAT] != 0
    val localHumanPlayerNumber get() = ints[LOCAL_HUMAN_PLAYER]
    val status get() = ints[STATUS]
    val numPlayers get() = ints[NUM_PLAYERS]
    val myPlayerNumber get() = ints[MY_PLAYER_NUMBER]
    val currentPlayer get() = ints[CURRENT_PLAYER]
    val playerCount get() = ints[PLAYER_COUNT]
    val serverPort get() = ints[SERVER_PORT]
    val longJumps get() = ints[LONG_JUMPS] != 0
    val hopOthers get() = ints[HOP_OTHERS] != 0
    val stopOthers get() = ints[STOP_OTHERS] != 0
    val extraComputerPlayerCount get() = ints[EXTRA_COMPUTER_PLAYERS]
    val computerPlayerCount get() = ints[COMPUTER_PLAYERS]
    val moveSerial get() = ints[MOVE_SERIAL]
    val moveNumber get() = ints[MOVE_NUMBER]
    val animationStepMs get() = ints[ANIMATION_STEP_MS]
    val animationDoneMs get() = ints[ANIMATION_DONE_MS]
    val computerSmarts get() = ints[COMPUTER_SMARTS]
    val webPort get() = ints[WEB_PORT]
    val webReady get() = ints[WEB_READY] != 0

    val holes: IntArray
        get() = ints.copyOfRange(HOLE_BASE, HOLE_BASE + HOLE_COUNT)

    val selected: IntArray
        get() = ints.copyOfRange(SELECTED_BASE, SELECTED_BASE + ints[SELECTED_COUNT])

    val telegraph: IntArray
        get() = ints.copyOfRange(TELEGRAPH_BASE, TELEGRAPH_BASE + ints[TELEGRAPH_COUNT])

    val lastMove: IntArray
        get() = ints.copyOfRange(LAST_MOVE_BASE, LAST_MOVE_BASE + ints[LAST_MOVE_COUNT])

    fun colorForPlayer(player: Int): Int =
        if (player in 1..6) ints[PLAYER_BASE + (player - 1) * PLAYER_STRIDE] else 0

    fun finishedForPlayer(player: Int): Boolean =
        player in 1..6 && ints[PLAYER_BASE + (player - 1) * PLAYER_STRIDE + 1] != 0

    fun finishedInMovesForPlayer(player: Int): Int =
        if (player in 1..6) ints[PLAYER_BASE + (player - 1) * PLAYER_STRIDE + 2] else 0

    fun movesTakenForPlayer(player: Int): Int =
        if (player in 1..6) ints[PLAYER_BASE + (player - 1) * PLAYER_STRIDE + 3] else 0

    fun pegsInGoalForPlayer(player: Int): Int =
        if (player in 1..6) ints[PLAYER_BASE + (player - 1) * PLAYER_STRIDE + 4] else 0

    fun nameForPlayer(player: Int): String =
        if (player in 1..6) names[player] else ""

    fun playerAtHole(hole: Int): Int =
        if (hole in holes.indices) holes[hole] else 0

    fun pegsForPlayer(player: Int): Int = holes.count { it == player }

    companion object {
        const val HOLE_COUNT = 247
        const val CONNECTED = 0
        const val IS_HOST = 1
        const val IS_SPECTATOR = 2
        const val ACTIVE_SEAT_KIND = 3
        const val HAS_LOCAL_HUMAN_SEAT = 4
        const val HAS_REMOTE_SEAT = 5
        const val LOCAL_HUMAN_PLAYER = 6
        const val STATUS = 7
        const val NUM_PLAYERS = 8
        const val MY_PLAYER_NUMBER = 9
        const val CURRENT_PLAYER = 10
        const val PLAYER_COUNT = 11
        const val SERVER_PORT = 12
        const val LONG_JUMPS = 13
        const val HOP_OTHERS = 14
        const val STOP_OTHERS = 15
        const val EXTRA_COMPUTER_PLAYERS = 16
        const val COMPUTER_PLAYERS = 17
        const val MOVE_SERIAL = 18
        const val MOVE_NUMBER = 19
        const val ANIMATION_STEP_MS = 20
        const val ANIMATION_DONE_MS = 21
        const val COMPUTER_SMARTS = 22
        const val WEB_PORT = 23
        const val WEB_READY = 24
        const val SELECTED_COUNT = 25
        const val TELEGRAPH_COUNT = 26
        const val LAST_MOVE_COUNT = 27
        const val PLAYER_BASE = 32
        const val PLAYER_STRIDE = 5
        const val HOLE_BASE = 62
        const val SELECTED_BASE = HOLE_BASE + HOLE_COUNT
        const val TELEGRAPH_BASE = SELECTED_BASE + 64
        const val LAST_MOVE_BASE = TELEGRAPH_BASE + 64
    }
}

@Keep
class CheechSession(private val listener: Listener) : CheechListener, AutoCloseable {
    interface Listener {
        fun onSnapshot(snapshot: CheechSnapshot)
        fun onMessage(message: String)
    }

    private val mainHandler = android.os.Handler(android.os.Looper.getMainLooper())
    private var handle: Long = nativeCreate()

    init {
        nativeSetListener(handle, this)
    }

    override fun onSnapshot(snapshot: CheechSnapshot) {
        mainHandler.post { listener.onSnapshot(snapshot) }
    }

    override fun onMessage(message: String) {
        mainHandler.post { listener.onMessage(message) }
    }

    fun joinHost(host: String, port: Int, spectator: Boolean, playerName: String, color: Int) =
        nativeJoinHost(handle, host, port, spectator, playerName, color)

    fun startGame(
        port: Int,
        numPlayers: Int,
        longJumps: Boolean,
        hopOthers: Boolean,
        stopOthers: Boolean,
        seats: List<CheechSeatSpec>,
        webPort: Int = 0,
    ) = nativeStartGame(
        handle, port, numPlayers, longJumps, hopOthers, stopOthers,
        seats.toTypedArray(), webPort,
    )

    fun resumeLocalGame(save: String, webPort: Int = 0): Boolean =
        nativeResumeLocalGame(handle, save, webPort)

    fun localGameSave(): String? = nativeLocalGameSave(handle)

    fun leave() = nativeLeave(handle)
    fun changeName(name: String) = nativeChangeName(handle, name)
    fun changeColor(color: Int) = nativeChangeColor(handle, color)
    fun sendChat(text: String) = nativeSendChat(handle, text)
    fun setAnimationStepMs(ms: Int) = nativeSetAnimationStepMs(handle, ms)
    fun setComputerSmarts(percent: Int) = nativeSetComputerSmarts(handle, percent)

    fun reconfigureGame(numPlayers: Int, longJumps: Boolean, hopOthers: Boolean, stopOthers: Boolean) =
        nativeReconfigureGame(handle, numPlayers, longJumps, hopOthers, stopOthers)

    fun addComputerPlayer(
        type: String,
        name: String,
        color: Int,
        friendPlayers: IntArray? = null,
        enemyPlayers: IntArray? = null,
    ) = nativeAddComputerPlayer(handle, type, name, color, friendPlayers, enemyPlayers)

    fun removeComputerPlayers() = nativeRemoveComputerPlayers(handle)
    fun undoMove() = nativeUndoMove(handle)
    fun restartGame() = nativeRestartGame(handle)
    fun rotatePlayers() = nativeRotatePlayers(handle)
    fun shufflePlayers() = nativeShufflePlayers(handle)
    fun tapHole(hole: Int) = nativeTapHole(handle, hole)
    fun confirmMove() = nativeConfirmMove(handle)
    fun clearSelection() = nativeClearSelection(handle)
    fun removeLastHop() = nativeRemoveLastHop(handle)
    fun consumeReplaySkip(): Boolean = nativeConsumeReplaySkip(handle)
    fun holdComputerMovesForAnimationHops(hops: Int) =
        nativeHoldComputerMoves(handle, hops)

    override fun close() {
        if (handle != 0L) {
            nativeDestroy(handle)
            handle = 0
        }
    }

    private external fun nativeCreate(): Long
    private external fun nativeDestroy(handle: Long)
    private external fun nativeSetListener(handle: Long, listener: CheechListener)

    private external fun nativeJoinHost(handle: Long, host: String, port: Int, spectator: Boolean, name: String, color: Int)
    private external fun nativeStartGame(
        handle: Long, port: Int, numPlayers: Int, longJumps: Boolean,
        hopOthers: Boolean, stopOthers: Boolean, seats: Array<CheechSeatSpec>, webPort: Int,
    )
    private external fun nativeResumeLocalGame(handle: Long, save: String, webPort: Int): Boolean
    private external fun nativeLocalGameSave(handle: Long): String?
    private external fun nativeLeave(handle: Long)
    private external fun nativeChangeName(handle: Long, name: String)
    private external fun nativeChangeColor(handle: Long, color: Int)
    private external fun nativeSendChat(handle: Long, text: String)
    private external fun nativeSetAnimationStepMs(handle: Long, ms: Int)
    private external fun nativeSetComputerSmarts(handle: Long, percent: Int)
    private external fun nativeReconfigureGame(handle: Long, numPlayers: Int, longJumps: Boolean, hopOthers: Boolean, stopOthers: Boolean)
    private external fun nativeAddComputerPlayer(
        handle: Long, type: String, name: String, color: Int,
        friendPlayers: IntArray?, enemyPlayers: IntArray?,
    )
    private external fun nativeRemoveComputerPlayers(handle: Long)
    private external fun nativeUndoMove(handle: Long)
    private external fun nativeRestartGame(handle: Long)
    private external fun nativeRotatePlayers(handle: Long)
    private external fun nativeShufflePlayers(handle: Long)
    private external fun nativeTapHole(handle: Long, hole: Int)
    private external fun nativeConfirmMove(handle: Long)
    private external fun nativeClearSelection(handle: Long)
    private external fun nativeRemoveLastHop(handle: Long)
    private external fun nativeConsumeReplaySkip(handle: Long): Boolean
    private external fun nativeHoldComputerMoves(handle: Long, hops: Int)

    companion object {
        init {
            System.loadLibrary("cheech")
        }

        @JvmStatic external fun boardHoleCount(): Int
        @JvmStatic external fun boardHolePresent(): BooleanArray
        @JvmStatic external fun boardHoleX(): DoubleArray
        @JvmStatic external fun boardHoleY(): DoubleArray
        @JvmStatic external fun defaultNameForComputerType(type: String): String
        @JvmStatic external fun typeNameForComputerType(type: String): String
        @JvmStatic external fun setWebRoot(path: String)
    }
}
