package com.octopilot.cheech.ui.game

import androidx.activity.compose.BackHandler
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.focusable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.imePadding
import androidx.compose.foundation.layout.navigationBarsPadding
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.statusBarsPadding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.layout.widthIn
import androidx.compose.foundation.layout.windowInsetsPadding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.LazyRow
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.lazy.rememberLazyListState
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Chat
import androidx.compose.material.icons.filled.Close
import androidx.compose.material.icons.filled.Edit
import androidx.compose.material.icons.filled.Fullscreen
import androidx.compose.material.icons.filled.FullscreenExit
import androidx.compose.material.icons.filled.MoreVert
import androidx.compose.material.icons.filled.Person
import androidx.compose.material.icons.filled.Send
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Button
import androidx.compose.material3.Card
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.FilterChip
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Switch
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.focus.FocusRequester
import androidx.compose.ui.focus.focusRequester
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.input.key.Key
import androidx.compose.ui.input.key.KeyEventType
import androidx.compose.ui.input.key.isCtrlPressed
import androidx.compose.ui.input.key.key
import androidx.compose.ui.input.key.onPreviewKeyEvent
import androidx.compose.ui.input.key.type
import androidx.compose.ui.platform.LocalSoftwareKeyboardController
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.window.Dialog
import androidx.compose.ui.window.DialogProperties
import androidx.core.view.WindowCompat
import androidx.core.view.WindowInsetsCompat
import androidx.core.view.WindowInsetsControllerCompat
import androidx.compose.ui.platform.LocalView
import androidx.compose.runtime.DisposableEffect
import com.octopilot.cheech.board.BoardView
import com.octopilot.cheech.board.PegColors
import com.octopilot.cheech.model.BotCatalog
import com.octopilot.cheech.model.ChatMessage
import com.octopilot.cheech.model.GameAction
import com.octopilot.cheech.model.SEAT_COMPUTER
import com.octopilot.cheech.model.SEAT_HUMAN
import com.octopilot.cheech.model.SessionModel
import com.octopilot.cheech.model.SetupMode
import com.octopilot.cheech.ui.ColorPickerRow
import com.octopilot.cheech.ui.InviteDialog
import com.octopilot.cheech.ui.LabeledValue
import com.octopilot.cheech.ui.RuleInfoButton
import com.octopilot.cheech.ui.SectionCard
import com.octopilot.cheech.ui.seatLabel

@Composable
fun GameScreen(model: SessionModel) {
    val snapshot = model.snapshot
    var menuOpen by remember { mutableStateOf(false) }
    var showProfile by remember { mutableStateOf(false) }
    var showSetup by remember { mutableStateOf(false) }
    var showInvite by remember { mutableStateOf(false) }
    val keyboardFocus = remember { FocusRequester() }
    val keyboardBlocked = showProfile || showSetup || showInvite || menuOpen ||
        model.chatOpen || model.confirmAction != null

    BackHandler(enabled = model.fullScreen) { model.fullScreen = false }
    BackHandler(enabled = !model.fullScreen && model.chatOpen) { model.chatOpen = false }
    BackHandler(enabled = !model.fullScreen && !model.chatOpen) {
        if (snapshot != null && model.canControl(snapshot) &&
            snapshot.selected.isNotEmpty()
        ) {
            model.clearSelection()
        } else {
            model.requestLeave()
        }
    }

    if (snapshot == null) {
        Box(Modifier.fillMaxSize(), contentAlignment = Alignment.Center) { Text("Connecting…") }
        return
    }

    LaunchedEffect(keyboardBlocked, model.fullScreen) {
        if (!keyboardBlocked) keyboardFocus.requestFocus()
    }

    Box(
        Modifier
            .fillMaxSize()
            .focusRequester(keyboardFocus)
            .focusable()
            .onPreviewKeyEvent { event ->
                if (!keyboardBlocked && event.key == Key.Escape) {
                    if (event.type == KeyEventType.KeyDown &&
                        model.canControl(snapshot) && snapshot.selected.isNotEmpty()
                    ) {
                        model.clearSelection()
                    }
                    // Consume the key-up too: Android re-dispatches an
                    // unhandled Escape as Back, which would leave the game.
                    return@onPreviewKeyEvent true
                }
                if (event.type != KeyEventType.KeyDown) return@onPreviewKeyEvent false
                if (keyboardBlocked) return@onPreviewKeyEvent false
                val canControl = model.canControl(snapshot)
                when {
                    event.isCtrlPressed && event.key == Key.Z -> {
                        if (snapshot.isHost && snapshot.hasLocalHumanSeat) {
                            model.undoMove()
                            true
                        } else false
                    }
                    event.key == Key.Enter -> {
                        if (canControl && snapshot.selected.size >= 2) {
                            model.confirmSelected()
                            true
                        } else false
                    }
                    else -> false
                }
            }
    ) {
        if (model.fullScreen) {
            FullscreenGame(model, snapshot, onExit = { model.fullScreen = false })
        } else {
        Column(
            Modifier
                .fillMaxSize()
                .background(MaterialTheme.colorScheme.background)
                .statusBarsPadding()
                .navigationBarsPadding()
        ) {
            Row(
                modifier = Modifier.fillMaxWidth().padding(horizontal = 6.dp, vertical = 2.dp),
                verticalAlignment = Alignment.CenterVertically,
            ) {
                IconButton(onClick = { model.requestLeave() }) {
                    Icon(Icons.Default.Close, contentDescription = "Leave game")
                }
                if (!snapshot.isHost && !snapshot.isSpectator) {
                    Row(
                        modifier = Modifier
                            .clip(RoundedCornerShape(20.dp))
                            .clickable { showProfile = true }
                            .padding(horizontal = 8.dp, vertical = 4.dp),
                        verticalAlignment = Alignment.CenterVertically,
                    ) {
                        Box(Modifier.size(14.dp).background(PegColors.of(model.playerColor), CircleShape))
                        Text(" ${model.playerName}", style = MaterialTheme.typography.labelLarge)
                        Icon(Icons.Default.Edit, contentDescription = "Edit profile", modifier = Modifier.size(14.dp))
                    }
                }
                Spacer(Modifier.weight(1f))
                if (snapshot.connected && !(snapshot.isHost && !snapshot.hasRemoteSeat)) {
                    Box {
                        IconButton(onClick = { model.chatOpen = true; model.markChatRead() }) {
                            Icon(Icons.Default.Chat, contentDescription = "Chat")
                        }
                        if (model.hasUnreadChat) {
                            Box(
                                Modifier
                                    .align(Alignment.TopEnd)
                                    .padding(8.dp)
                                    .size(8.dp)
                                    .background(MaterialTheme.colorScheme.error, CircleShape)
                            )
                        }
                    }
                }
                Box {
                    IconButton(onClick = { menuOpen = true }) {
                        Icon(Icons.Default.MoreVert, contentDescription = "Game menu")
                    }
                    DropdownMenu(expanded = menuOpen, onDismissRequest = { menuOpen = false }) {
                        if (snapshot.isHost && snapshot.hasLocalHumanSeat) {
                            DropdownMenuItem(text = { Text("Undo") }, onClick = { menuOpen = false; model.undoMove() })
                            DropdownMenuItem(text = { Text("Restart") }, onClick = { menuOpen = false; model.requestAction(GameAction.Restart) })
                            DropdownMenuItem(text = { Text("Rotate") }, onClick = { menuOpen = false; model.requestAction(GameAction.Rotate) })
                            DropdownMenuItem(text = { Text("Shuffle") }, onClick = { menuOpen = false; model.requestAction(GameAction.Shuffle) })
                        }
                        if (snapshot.isHost) {
                            DropdownMenuItem(text = { Text("Setup Game") }, onClick = { menuOpen = false; showSetup = true })
                            if (snapshot.hasRemoteSeat) {
                                DropdownMenuItem(text = { Text("Invite More Players") }, onClick = { menuOpen = false; showInvite = true })
                            }
                        }
                        DropdownMenuItem(text = { Text("Leave Game") }, onClick = { menuOpen = false; model.requestLeave() })
                    }
                }
                IconButton(onClick = { model.fullScreen = true }) {
                    Icon(Icons.Default.Fullscreen, contentDescription = "Full screen")
                }
            }

            PlayerList(model, snapshot)

            Row(
                modifier = Modifier.fillMaxWidth().padding(horizontal = 18.dp, vertical = 4.dp),
                verticalAlignment = Alignment.CenterVertically,
            ) {
                val turn = model.displayedPlayer(snapshot)
                Box(Modifier.size(12.dp).background(PegColors.of(snapshot.colorForPlayer(turn)), CircleShape))
                Text(
                    "  ${snapshot.nameForPlayer(turn).ifEmpty { if (turn == snapshot.myPlayerNumber) "You" else "Player $turn" }}",
                    style = MaterialTheme.typography.titleSmall,
                    fontWeight = FontWeight.SemiBold,
                )
                Spacer(Modifier.weight(1f))
                Text("Move ${snapshot.moveNumber}", style = MaterialTheme.typography.labelLarge, color = MaterialTheme.colorScheme.onSurfaceVariant)
                if (snapshot.isSpectator) Text("  · Spectator", style = MaterialTheme.typography.labelSmall)
                if (snapshot.webReady) Text("  · Web", style = MaterialTheme.typography.labelSmall, color = MaterialTheme.colorScheme.primary)
            }

            Box(Modifier.weight(1f).fillMaxWidth()) {
                BoardView(model, Modifier.fillMaxSize().padding(4.dp))
                MoveControls(model, snapshot, Modifier.align(Alignment.BottomEnd).padding(18.dp))
                val winner = model.winner(snapshot)
                if (winner > 0) {
                    WinnerOverlay(model, snapshot, winner, Modifier.align(Alignment.Center))
                }
                if (model.chatOpen) {
                    ChatPanel(model, Modifier.align(Alignment.TopCenter).padding(12.dp))
                }
            }
        }
    }
    }

    if (showProfile) {
        ProfileEditor(model) { showProfile = false }
    }
    if (showSetup) {
        GameSetupDialog(model) { showSetup = false }
    }
    if (showInvite) InviteDialog(model) { showInvite = false }

    model.confirmAction?.let { action ->
        AlertDialog(
            onDismissRequest = { model.confirmAction = null },
            confirmButton = { TextButton(onClick = { model.perform(action) }) { Text(actionLabel(action)) } },
            dismissButton = { TextButton(onClick = { model.confirmAction = null }) { Text("Cancel") } },
            title = { Text(actionLabel(action)) },
            text = { Text("This will interrupt the current game. Continue?") },
        )
    }
}

private fun actionLabel(action: GameAction): String = when (action) {
    GameAction.Leave -> "Leave Game"
    GameAction.Restart -> "Restart Game"
    GameAction.Rotate -> "Rotate Players"
    GameAction.Shuffle -> "Shuffle Players"
}

@Composable
private fun PlayerList(model: SessionModel, snapshot: com.octopilot.cheech.bridge.CheechSnapshot) {
    LazyRow(
        modifier = Modifier.fillMaxWidth().padding(horizontal = 12.dp),
        horizontalArrangement = Arrangement.spacedBy(8.dp),
    ) {
        items((1..snapshot.numPlayers).toList()) { player ->
            val name = snapshot.nameForPlayer(player)
            val current = model.displayedPlayer(snapshot) == player
            Card(
                modifier = Modifier.widthIn(min = 104.dp),
                colors = androidx.compose.material3.CardDefaults.cardColors(
                    containerColor = if (current) MaterialTheme.colorScheme.primaryContainer
                    else MaterialTheme.colorScheme.surfaceVariant
                ),
            ) {
                Column(Modifier.padding(8.dp)) {
                    Row(verticalAlignment = Alignment.CenterVertically) {
                        Box(Modifier.size(12.dp).background(PegColors.of(snapshot.colorForPlayer(player)), CircleShape))
                        Text(" ${name.ifEmpty { "Open seat" }}", style = MaterialTheme.typography.labelLarge, maxLines = 1)
                    }
                    if (snapshot.finishedForPlayer(player)) {
                        Text("Finished in ${snapshot.finishedInMovesForPlayer(player)}", style = MaterialTheme.typography.labelSmall)
                    } else {
                        Text("${snapshot.movesTakenForPlayer(player)} moves", style = MaterialTheme.typography.labelSmall)
                    }
                }
            }
        }
    }
}

@Composable
private fun MoveControls(
    model: SessionModel,
    snapshot: com.octopilot.cheech.bridge.CheechSnapshot,
    modifier: Modifier = Modifier,
) {
    val canControl = model.canControl(snapshot)
    if (!canControl || snapshot.selected.isEmpty()) return
    Row(modifier, horizontalArrangement = Arrangement.spacedBy(8.dp)) {
        OutlinedButton(onClick = { model.clearSelection() }) { Text("Cancel") }
        Button(
            onClick = { model.confirmSelected() },
            enabled = snapshot.selected.size >= 2,
        ) { Text("Move") }
    }
}

@Composable
private fun WinnerOverlay(
    model: SessionModel,
    snapshot: com.octopilot.cheech.bridge.CheechSnapshot,
    winner: Int,
    modifier: Modifier = Modifier,
) {
    Card(
        modifier = modifier.padding(24.dp),
        shape = RoundedCornerShape(20.dp),
    ) {
        Column(Modifier.padding(20.dp), horizontalAlignment = Alignment.CenterHorizontally) {
            Text("${snapshot.nameForPlayer(winner).ifEmpty { "Player $winner" }} wins!", style = MaterialTheme.typography.headlineSmall, fontWeight = FontWeight.Bold)
            Spacer(Modifier.height(12.dp))
            if (snapshot.isHost) {
                Button(onClick = { model.requestAction(GameAction.Restart) }) { Text("Play Again") }
            }
        }
    }
}

@Composable
private fun FullscreenGame(
    model: SessionModel,
    snapshot: com.octopilot.cheech.bridge.CheechSnapshot,
    onExit: () -> Unit,
) {
    val view = LocalView.current
    DisposableEffect(Unit) {
        val window = (view.context as android.app.Activity).window
        WindowCompat.getInsetsController(window, view).apply {
            hide(WindowInsetsCompat.Type.systemBars())
            systemBarsBehavior = WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE
        }
        onDispose {
            WindowCompat.getInsetsController(window, view).show(WindowInsetsCompat.Type.systemBars())
        }
    }
    Box(Modifier.fillMaxSize().background(Color.Black)) {
        BoardView(model, Modifier.fillMaxSize())
        IconButton(
            onClick = onExit,
            modifier = Modifier.align(Alignment.TopEnd).statusBarsPadding().padding(8.dp),
        ) { Icon(Icons.Default.FullscreenExit, contentDescription = "Exit full screen", tint = Color.White) }
        MoveControls(model, snapshot, Modifier.align(Alignment.BottomEnd).padding(18.dp))
    }
}

@Composable
private fun ChatPanel(model: SessionModel, modifier: Modifier = Modifier) {
    val listState = rememberLazyListState()
    var draft by remember { mutableStateOf("") }
    val focus = remember { FocusRequester() }
    val keyboard = LocalSoftwareKeyboardController.current
    LaunchedEffect(Unit) {
        model.markChatRead()
        focus.requestFocus()
    }
    LaunchedEffect(model.messages.size) {
        if (model.messages.isNotEmpty()) listState.animateScrollToItem(model.messages.lastIndex)
    }
    Card(modifier.fillMaxWidth().heightIn(max = 320.dp), shape = RoundedCornerShape(18.dp)) {
        Column(Modifier.padding(12.dp)) {
            Row(verticalAlignment = Alignment.CenterVertically) {
                Text("Chat", style = MaterialTheme.typography.titleMedium, fontWeight = FontWeight.SemiBold, modifier = Modifier.weight(1f))
                IconButton(onClick = { model.chatOpen = false }) { Icon(Icons.Default.Close, contentDescription = "Close chat") }
            }
            LazyColumn(state = listState, modifier = Modifier.weight(1f, fill = false).heightIn(max = 220.dp)) {
                items(model.messages) { message: ChatMessage ->
                    Text(
                        message.text,
                        style = MaterialTheme.typography.bodySmall,
                        color = if (message.system) MaterialTheme.colorScheme.onSurfaceVariant else MaterialTheme.colorScheme.onSurface,
                        modifier = Modifier.padding(vertical = 2.dp),
                    )
                }
            }
            Row(verticalAlignment = Alignment.CenterVertically) {
                OutlinedTextField(
                    value = draft,
                    onValueChange = { draft = it },
                    placeholder = { Text("Message") },
                    singleLine = true,
                    modifier = Modifier.weight(1f).focusRequester(focus),
                )
                IconButton(onClick = {
                    model.sendChat(draft)
                    draft = ""
                    keyboard?.hide()
                }) { Icon(Icons.Default.Send, contentDescription = "Send") }
            }
        }
    }
}

@Composable
private fun ProfileEditor(model: SessionModel, onDismiss: () -> Unit) {
    var name by remember { mutableStateOf(model.playerName) }
    var color by remember { mutableStateOf(model.playerColor) }
    AlertDialog(
        onDismissRequest = onDismiss,
        confirmButton = {
            TextButton(onClick = {
                model.updateProfile(name, color)
                onDismiss()
            }) { Text("Done") }
        },
        dismissButton = { TextButton(onClick = onDismiss) { Text("Cancel") } },
        title = { Text("Player Profile") },
        text = {
            Column(verticalArrangement = Arrangement.spacedBy(12.dp)) {
                OutlinedTextField(value = name, onValueChange = { name = it }, label = { Text("Name") }, singleLine = true)
                Text("Color")
                ColorPickerRow(color) { color = it }
            }
        },
    )
}

@Composable
private fun GameSetupDialog(model: SessionModel, onDismiss: () -> Unit) {
    val snapshot = model.snapshot
    var players by remember { mutableStateOf(model.numPlayers) }
    var jumps by remember { mutableStateOf(model.longJumps) }
    var hop by remember { mutableStateOf(model.hopOthers) }
    var stop by remember { mutableStateOf(model.stopOthers) }
    var botType by remember { mutableStateOf(model.lastBotType) }
    val initial = remember { listOf(model.numPlayers, model.longJumps, model.hopOthers, model.stopOthers) }
    val changed = players != initial[0] || jumps != initial[1] || hop != initial[2] || stop != initial[3]

    Dialog(onDismissRequest = onDismiss, properties = DialogProperties(usePlatformDefaultWidth = false)) {
        Card(Modifier.fillMaxWidth().padding(20.dp), shape = RoundedCornerShape(22.dp)) {
            Column(Modifier.padding(18.dp), verticalArrangement = Arrangement.spacedBy(12.dp)) {
                Text("Game Setup", style = MaterialTheme.typography.titleLarge, fontWeight = FontWeight.Bold)
                Row(verticalAlignment = Alignment.CenterVertically) {
                    Text("Players", Modifier.weight(1f))
                    OutlinedButton(onClick = { players = (players - 1).coerceAtLeast(2) }) { Text("−") }
                    Text(" $players ", modifier = Modifier.padding(horizontal = 8.dp))
                    OutlinedButton(onClick = { players = (players + 1).coerceAtMost(6) }) { Text("+") }
                }
                Row(verticalAlignment = Alignment.CenterVertically) {
                    Text("Long jumps", Modifier.weight(1f))
                    Switch(checked = jumps, onCheckedChange = { jumps = it })
                }
                Row(verticalAlignment = Alignment.CenterVertically) {
                    Text("Enter opponents' goals", Modifier.weight(1f))
                    Switch(checked = hop, onCheckedChange = { hop = it })
                }
                Row(verticalAlignment = Alignment.CenterVertically) {
                    Text("Stop in opponents' goals", Modifier.weight(1f))
                    Switch(checked = stop, onCheckedChange = { stop = it })
                }
                Text("Computer settings", fontWeight = FontWeight.SemiBold)
                Row(horizontalArrangement = Arrangement.spacedBy(6.dp)) {
                    BotCatalog.types.forEach { type ->
                        FilterChip(selected = botType == type, onClick = { botType = type }, label = { Text(BotCatalog.familyName(type)) })
                    }
                }
                Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                    OutlinedButton(onClick = { model.addComputerPlayer(botType, "", 0) }) { Text("Add Computer") }
                    OutlinedButton(onClick = { model.removeComputerPlayers() }) { Text("Remove All") }
                }
                if (snapshot != null) {
                    LabeledValue("Connected", "${snapshot.playerCount} / ${snapshot.numPlayers}")
                    LabeledValue("Added by you", "${snapshot.extraComputerPlayerCount}")
                }
                Row(horizontalArrangement = Arrangement.spacedBy(8.dp), modifier = Modifier.fillMaxWidth()) {
                    TextButton(onClick = onDismiss, modifier = Modifier.weight(1f)) { Text("Cancel") }
                    Button(
                        onClick = {
                            model.reconfigureGame(players, jumps, hop, stop)
                            onDismiss()
                        },
                        enabled = changed,
                        modifier = Modifier.weight(1f),
                    ) { Text("Apply") }
                }
            }
        }
    }
}
