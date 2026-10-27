package com.octopilot.cheech.ui

import android.content.ClipData
import android.content.ClipboardManager
import android.content.Context
import android.content.Intent
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Info
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Button
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import com.octopilot.cheech.board.PegColors
import com.octopilot.cheech.model.SessionModel
import com.octopilot.cheech.util.localIPAddress

@Composable
fun ColorPickerRow(selected: Int, onSelect: (Int) -> Unit) {
    Row(horizontalArrangement = Arrangement.spacedBy(8.dp), verticalAlignment = Alignment.CenterVertically) {
        for (color in 1..8) {
            val isSelected = selected == color
            val fillAlpha = if (isSelected) 1f else 0.5f
            val borderAlpha = if (isSelected) 1f else 0.5f
            Box(
                modifier = Modifier
                    .size(30.dp)
                    .clip(CircleShape)
                    .background(PegColors.of(color).copy(alpha = fillAlpha))
                    .border(
                        width = if (isSelected) 3.dp else 1.dp,
                        color = if (isSelected) MaterialTheme.colorScheme.primary
                        else Color.Black.copy(alpha = borderAlpha),
                        shape = CircleShape,
                    )
                    .clickable { onSelect(color) }
            )
        }
    }
}

@Composable
fun RuleInfoButton(title: String, text: String) {
    var open by remember { mutableStateOf(false) }
    IconButton(onClick = { open = true }, modifier = Modifier.size(28.dp)) {
        Icon(Icons.Default.Info, contentDescription = "About $title", tint = MaterialTheme.colorScheme.primary)
    }
    if (open) {
        AlertDialog(
            onDismissRequest = { open = false },
            confirmButton = { TextButton(onClick = { open = false }) { Text("OK") } },
            title = { Text(title) },
            text = { Text(text) },
        )
    }
}

@Composable
fun HowToPlayDialog(onDismiss: () -> Unit) {
    AlertDialog(
        onDismissRequest = onDismiss,
        confirmButton = { TextButton(onClick = onDismiss) { Text("Done") } },
        title = { Text("How to Play") },
        text = {
            Column(verticalArrangement = Arrangement.spacedBy(10.dp)) {
                Text("Move all ten of your pegs into the opposite point of the star.", fontWeight = FontWeight.SemiBold)
                Text("Tap one of your pegs, then tap an empty hole to move. A peg moves one step to an adjacent hole, or jumps over a neighbouring peg into the empty hole beyond. Chain jumps together for a longer move.")
                Text("Long jumps may skip empty holes. Rules variations let you enter or stop inside an opponent's goal.")
                Text("Press Move to commit the path. Press Cancel to clear it. Tap the last hole again quickly to confirm.")
            }
        },
    )
}

@Composable
fun InviteDialog(model: SessionModel, onDismiss: () -> Unit) {
    val context = LocalContext.current
    val snapshot = model.snapshot
    val host = remember(snapshot?.serverHost, snapshot?.serverPort, model.hostPort) {
        val shown = snapshot?.serverHost.orEmpty()
        if (shown.isEmpty() || shown == "127.0.0.1" || shown == "localhost") {
            localIPAddress() ?: "Unavailable"
        } else shown
    }
    val port = if ((snapshot?.serverPort ?: 0) > 0) snapshot!!.serverPort else model.hostPort
    val url = "cheech://join?host=$host&port=$port"
    var editablePort by remember { mutableStateOf(port.toString()) }

    AlertDialog(
        onDismissRequest = onDismiss,
        confirmButton = { TextButton(onClick = onDismiss) { Text("Done") } },
        title = { Text("Invite a Friend") },
        text = {
            Column(verticalArrangement = Arrangement.spacedBy(12.dp)) {
                Text("Share this address with another device on the same network.")
                Text("Host: $host")
                OutlinedTextField(
                    value = editablePort,
                    onValueChange = { value ->
                        editablePort = value.filter { it.isDigit() }.take(5)
                        editablePort.toIntOrNull()?.let { model.hostPort = it }
                    },
                    label = { Text("Port") },
                    singleLine = true,
                    modifier = Modifier.fillMaxWidth(),
                )
                Text(url, style = MaterialTheme.typography.bodySmall)
                Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                    Button(onClick = {
                        val send = Intent(Intent.ACTION_SEND).apply {
                            type = "text/plain"
                            putExtra(Intent.EXTRA_TEXT, "Join my Cheech game: $url")
                        }
                        context.startActivity(Intent.createChooser(send, "Invite a friend"))
                    }) { Text("Share") }
                    OutlinedButton(onClick = {
                        val clipboard = context.getSystemService(Context.CLIPBOARD_SERVICE) as ClipboardManager
                        clipboard.setPrimaryClip(ClipData.newPlainText("Cheech invite", url))
                    }) { Text("Copy Link") }
                }
                if (host == "Unavailable") {
                    Text("No local network address found. Connect to Wi-Fi to invite another device.", color = MaterialTheme.colorScheme.error)
                }
            }
        },
    )
}

@Composable
fun SectionCard(title: String, content: @Composable () -> Unit) {
    SectionCard {
        Text(title, style = MaterialTheme.typography.titleMedium, fontWeight = FontWeight.SemiBold)
        content()
    }
}

@Composable
fun SectionCard(content: @Composable () -> Unit) {
    Column(
        modifier = Modifier
            .fillMaxWidth()
            .clip(RoundedCornerShape(18.dp))
            .background(MaterialTheme.colorScheme.surfaceVariant.copy(alpha = 0.5f))
            .padding(16.dp),
        verticalArrangement = Arrangement.spacedBy(10.dp),
    ) {
        content()
    }
}

@Composable
fun LabeledValue(label: String, value: String) {
    Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.SpaceBetween) {
        Text(label, color = MaterialTheme.colorScheme.onSurfaceVariant)
        Text(value, fontWeight = FontWeight.Medium)
    }
}

@Composable
fun SmallSpacer() = Spacer(Modifier.height(4.dp))
