package com.octopilot.cheech.ui

import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.safeDrawingPadding
import androidx.compose.foundation.layout.widthIn
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.itemsIndexed
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.ArrowDropDown
import androidx.compose.material.icons.filled.Info
import androidx.compose.material.icons.filled.PersonAdd
import androidx.compose.material.icons.filled.Settings
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Button
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.FilterChip
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Slider
import androidx.compose.material3.Switch
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import com.octopilot.cheech.model.BotCatalog
import com.octopilot.cheech.model.BotSkill
import com.octopilot.cheech.model.SEAT_COMPUTER
import com.octopilot.cheech.model.SEAT_HUMAN
import com.octopilot.cheech.model.SEAT_REMOTE
import com.octopilot.cheech.model.SeatConfig
import com.octopilot.cheech.model.SessionModel
import com.octopilot.cheech.model.SetupMode
import kotlin.math.roundToInt

@Composable
fun SetupScreen(model: SessionModel) {
    var showHowTo by remember { mutableStateOf(false) }
    var showInvite by remember { mutableStateOf(false) }

    Box(Modifier.fillMaxSize(), contentAlignment = Alignment.TopCenter) {
        Column(
            modifier = Modifier
                .fillMaxHeight()
                .widthIn(max = 720.dp)
                .fillMaxWidth()
                .safeDrawingPadding()
                .padding(horizontal = 20.dp),
        ) {
            Row(
                modifier = Modifier.fillMaxWidth().padding(top = 8.dp),
                verticalAlignment = Alignment.CenterVertically,
            ) {
                Column(Modifier.weight(1f)) {
                    Text("Cheech", style = MaterialTheme.typography.headlineLarge, fontWeight = FontWeight.Bold)
                    Text("Chinese Checkers", style = MaterialTheme.typography.titleMedium, color = MaterialTheme.colorScheme.onSurfaceVariant)
                }
                IconButton(onClick = { showHowTo = true }) {
                    Icon(Icons.Default.Info, contentDescription = "How to play")
                }
            }

            Button(
                onClick = {
                    if (model.setupMode == SetupMode.Start) model.startGame() else model.joinGame()
                },
                modifier = Modifier.fillMaxWidth().padding(top = 10.dp).height(54.dp),
                shape = RoundedCornerShape(16.dp),
            ) {
                Text(
                    if (model.setupMode == SetupMode.Start) "Start Game" else "Join Game",
                    style = MaterialTheme.typography.titleMedium,
                )
            }

            LazyColumn(
                modifier = Modifier.fillMaxWidth().weight(1f).padding(top = 14.dp),
                verticalArrangement = Arrangement.spacedBy(14.dp),
                contentPadding = PaddingValues(bottom = 24.dp),
            ) {
            item {
                Row(
                    modifier = Modifier.padding(top = 2.dp),
                    horizontalArrangement = Arrangement.spacedBy(8.dp),
                ) {
                    FilterChip(
                        selected = model.setupMode == SetupMode.Start,
                        onClick = { model.setupMode = SetupMode.Start },
                        label = { Text("Start Game") },
                    )
                    FilterChip(
                        selected = model.setupMode == SetupMode.Join,
                        onClick = { model.setupMode = SetupMode.Join },
                        label = { Text("Join Game") },
                    )
                }
            }
            if (model.setupMode == SetupMode.Start) {
                item {
                    SectionCard("Rules") {
                        Row(verticalAlignment = Alignment.CenterVertically) {
                            Text("Players", Modifier.weight(1f))
                            OutlinedButton(onClick = {
                                model.numPlayers = (model.numPlayers - 1).coerceAtLeast(2)
                                model.syncSeatCount(model.numPlayers)
                            }, modifier = Modifier.widthIn(min = 42.dp)) { Text("−") }
                            Text("${model.numPlayers}", modifier = Modifier.padding(horizontal = 12.dp))
                            OutlinedButton(onClick = {
                                model.numPlayers = (model.numPlayers + 1).coerceAtMost(6)
                                model.syncSeatCount(model.numPlayers)
                            }, modifier = Modifier.widthIn(min = 42.dp)) { Text("+") }
                        }
                        RuleToggle("Allow long jumps", model.longJumps) { model.longJumps = it }
                        RuleToggle("Can enter opponents' goals", model.hopOthers) { model.hopOthers = it }
                        if (model.hopOthers) {
                            RuleSwitchRow("Can stop in opponents' goal", model.stopOthers) { model.stopOthers = it }
                        }
                        Text("Animation Speed: ${if (model.opponentStepMs == 0) "Instant" else "${model.opponentStepMs} ms"}")
                        Slider(
                            value = model.opponentStepMs.toFloat(),
                            onValueChange = { model.opponentStepMs = (it / 10).roundToInt() * 10 },
                            valueRange = 0f..500f,
                            steps = 49,
                        )
                    }
                }
                item {
                    Row(verticalAlignment = Alignment.CenterVertically) {
                        Text("Players", style = MaterialTheme.typography.titleLarge, fontWeight = FontWeight.SemiBold, modifier = Modifier.weight(1f))
                        if (model.seats.any { it.kind == SEAT_REMOTE }) {
                            OutlinedButton(onClick = { showInvite = true }) { Text("Invite a Friend") }
                        }
                    }
                }
                itemsIndexed(model.seats) { index, seat ->
                    SeatEditor(model, index, seat)
                }
                item { Spacer(Modifier.height(28.dp)) }
            } else {
                item {
                    SectionCard("Server") {
                        OutlinedTextField(
                            value = model.joinHost,
                            onValueChange = { model.joinHost = it },
                            label = { Text("Server host") },
                            singleLine = true,
                            modifier = Modifier.fillMaxWidth(),
                        )
                        OutlinedTextField(
                            value = model.joinPort.toString(),
                            onValueChange = { text -> valueOrNull(text)?.let { model.joinPort = it.coerceIn(1, 65535) } },
                            label = { Text("Port") },
                            singleLine = true,
                            modifier = Modifier.fillMaxWidth(),
                        )
                    }
                }
                item {
                    SectionCard("You") {
                        OutlinedTextField(
                            value = model.playerName,
                            onValueChange = { model.playerName = it },
                            label = { Text("Player name") },
                            singleLine = true,
                            modifier = Modifier.fillMaxWidth(),
                        )
                        Text("Color")
                        ColorPickerRow(model.playerColor) { model.playerColor = it }
                        Row(verticalAlignment = Alignment.CenterVertically, modifier = Modifier.fillMaxWidth()) {
                            Column(Modifier.weight(1f)) {
                                Text("Join as spectator")
                                Text("Watch without taking a seat", style = MaterialTheme.typography.bodySmall, color = MaterialTheme.colorScheme.onSurfaceVariant)
                            }
                            Switch(checked = model.joinSpectator, onCheckedChange = { model.joinSpectator = it })
                        }
                    }
                }
                item { Spacer(Modifier.height(28.dp)) }
            }
            }
        }
    }

    if (showHowTo) HowToPlayDialog { showHowTo = false }
    if (showInvite) InviteDialog(model) { showInvite = false }
    model.errorText?.let { error ->
        AlertDialog(
            onDismissRequest = { model.errorText = null },
            confirmButton = { TextButton(onClick = { model.errorText = null }) { Text("OK") } },
            title = { Text("Could not join") },
            text = { Text(error) },
        )
    }
}

@Composable
private fun RuleToggle(label: String, checked: Boolean, onChange: (Boolean) -> Unit) {
    RuleSwitchRow(label, checked, onChange)
}

@Composable
private fun RuleSwitchRow(label: String, checked: Boolean, onChange: (Boolean) -> Unit) {
    Row(verticalAlignment = Alignment.CenterVertically, modifier = Modifier.fillMaxWidth()) {
        Text(label, Modifier.weight(1f))
        RuleInfoButton(
            label,
            "This changes how pegs may move on the board. The server enforces the selected variation for every player.",
        )
        Switch(checked = checked, onCheckedChange = onChange)
    }
}

@Composable
private fun SeatEditor(model: SessionModel, index: Int, seat: SeatConfig) {
    SectionCard {
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.spacedBy(8.dp),
        ) {
            listOf(SEAT_HUMAN to "Human", SEAT_COMPUTER to "Computer", SEAT_REMOTE to "Remote").forEach { (kind, label) ->
                FilterChip(
                    selected = seat.kind == kind,
                    modifier = Modifier.weight(1f),
                    onClick = {
                        model.updateSeat(index) {
                            it.copy(
                                kind = kind,
                                name = if (kind == SEAT_COMPUTER) "" else it.name,
                                humanName = if (kind == SEAT_HUMAN) it.humanName ?: "Player ${index + 1}" else it.humanName,
                            )
                        }
                    },
                    label = { Text(label) },
                )
            }
        }
        when (seat.kind) {
            SEAT_HUMAN -> {
                ColorPickerRow(seat.color) { model.setColor(index, it) }
                OutlinedTextField(
                    value = seat.humanName.orEmpty(),
                    onValueChange = { value -> model.updateSeat(index) { it.copy(humanName = value) } },
                    label = { Text("Name") },
                    singleLine = true,
                    modifier = Modifier.fillMaxWidth(),
                )
            }
            SEAT_COMPUTER -> {
                ColorPickerRow(seat.color) { model.setColor(index, it) }
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    verticalAlignment = Alignment.CenterVertically,
                    horizontalArrangement = Arrangement.spacedBy(6.dp),
                ) {
                    Text("Bot Type", Modifier.weight(1f))
                    BotTypeMenu(model, index, seat)
                    BotDetailsDialog(model, index, seat)
                }
            }
            else -> {
                Text("This seat is left open for another device. Share an invite link after starting the game.", color = MaterialTheme.colorScheme.onSurfaceVariant)
            }
        }
    }
}

@Composable
private fun BotTypeMenu(model: SessionModel, index: Int, seat: SeatConfig) {
    var open by remember { mutableStateOf(false) }
    Box {
        OutlinedButton(onClick = { open = true }) { Text(BotCatalog.familyName(seat.botType)) }
        DropdownMenu(expanded = open, onDismissRequest = { open = false }) {
            for (type in BotCatalog.types) {
                DropdownMenuItem(
                    text = { Text(BotCatalog.familyName(type)) },
                    onClick = {
                        open = false
                        model.lastBotType = type
                        model.updateSeat(index) {
                            it.copy(botType = type, name = "", skill = it.skill, depth = null)
                        }
                    },
                )
            }
        }
    }
}

private fun depthLabel(depth: Int?): String = when (depth) {
    null -> "Default"
    5 -> "5 (Slow)"
    else -> depth.toString()
}

@Composable
private fun ChoiceDropdown(value: String, options: List<Pair<String, () -> Unit>>) {
    var open by remember { mutableStateOf(false) }
    Box(Modifier.fillMaxWidth()) {
        OutlinedButton(
            onClick = { open = true },
            modifier = Modifier.fillMaxWidth(),
            contentPadding = PaddingValues(horizontal = 12.dp, vertical = 10.dp),
        ) {
            Text(value, Modifier.weight(1f), textAlign = TextAlign.Start)
            Icon(Icons.Default.ArrowDropDown, contentDescription = null)
        }
        DropdownMenu(
            expanded = open,
            onDismissRequest = { open = false },
            modifier = Modifier.fillMaxWidth(0.9f),
        ) {
            options.forEach { (label, select) ->
                DropdownMenuItem(
                    text = { Text(label) },
                    onClick = {
                        open = false
                        select()
                    },
                )
            }
        }
    }
}

@Composable
private fun BotDetailsDialog(model: SessionModel, index: Int, seat: SeatConfig) {
    var open by remember { mutableStateOf(false) }
    OutlinedButton(
        onClick = { open = true },
        contentPadding = PaddingValues(horizontal = 12.dp, vertical = 8.dp),
    ) {
        Icon(Icons.Default.Settings, contentDescription = "Bot settings")
    }
    if (!open) return
    AlertDialog(
        onDismissRequest = { open = false },
        confirmButton = { TextButton(onClick = { open = false }) { Text("Done") } },
        title = { Text(BotCatalog.familyName(seat.botType)) },
        text = {
            Column(verticalArrangement = Arrangement.spacedBy(12.dp)) {
                Text(
                    when {
                        seat.botType.startsWith("Mean") -> "Plays against everyone, trying to block opponents while racing to the goal."
                        seat.botType.startsWith("Friendly") -> "Cooperates with the players you choose and avoids blocking them."
                        else -> "Plans several of its own moves ahead, treating opponents as obstacles."
                    },
                    style = MaterialTheme.typography.bodySmall,
                )
                Text("Skill", fontWeight = FontWeight.SemiBold)
                ChoiceDropdown(
                    value = BotSkill.fromLevel(seat.skill).label,
                    options = BotSkill.entries.map { skill ->
                        skill.label to { model.updateSeat(index) { it.copy(skill = skill.level) } }
                    },
                )
                Text("Search depth", fontWeight = FontWeight.SemiBold)
                ChoiceDropdown(
                    value = depthLabel(seat.depth),
                    options = listOf<Int?>(null, 2, 3, 4, 5).map { depth ->
                        depthLabel(depth) to { model.updateSeat(index) { it.copy(depth = depth) } }
                    },
                )
                if (model.seats.size > 2 && seat.botType.startsWith("Friendly")) {
                    FocusList("Friendly to", model, index, seat, friend = true)
                }
                if (model.seats.size > 2 && seat.botType.startsWith("Mean")) {
                    FocusList("Mean to", model, index, seat, friend = false)
                }
            }
        },
    )
}

@Composable
private fun FocusList(label: String, model: SessionModel, index: Int, seat: SeatConfig, friend: Boolean) {
    Text(label, fontWeight = FontWeight.SemiBold)
    for (otherIndex in model.seats.indices) {
        if (otherIndex == index) continue
        val other = model.seats[otherIndex]
        val otherIds = if (friend) seat.friendSeats else seat.enemySeats
        val selected = otherIds == null || otherIds.contains(other.id)
        Row(
            verticalAlignment = Alignment.CenterVertically,
            modifier = Modifier.fillMaxWidth().clickable {
                model.updateSeat(index) {
                    val current = (if (friend) it.friendSeats else it.enemySeats)?.toMutableList()
                        ?: model.seats.map { s -> s.id }.toMutableList()
                    if (selected) current.remove(other.id) else current.add(other.id)
                    if (friend) it.copy(friendSeats = current) else it.copy(enemySeats = current)
                }
            }.padding(vertical = 3.dp),
        ) {
            androidx.compose.material3.Checkbox(checked = selected, onCheckedChange = null)
            Text(seatLabel(other, otherIndex), Modifier.weight(1f))
        }
    }
}

fun seatLabel(seat: SeatConfig, index: Int): String = when (seat.kind) {
    SEAT_HUMAN -> seat.humanName?.ifEmpty { "Player ${index + 1}" } ?: "Player ${index + 1}"
    SEAT_COMPUTER -> BotCatalog.familyName(seat.botType)
    else -> "Open seat ${index + 1}"
}

private fun valueOrNull(text: String): Int? = text.toIntOrNull()

@Composable
fun BotColorDot(color: Int) {
    Box(
        Modifier
            .padding(2.dp)
            .height(10.dp)
            .widthIn(min = 10.dp)
            .clickable(enabled = false) {},
    ) {
        androidx.compose.foundation.Canvas(Modifier.fillMaxSize()) {
            drawCircle(com.octopilot.cheech.board.PegColors.of(color), radius = size.minDimension / 2)
        }
    }
}
