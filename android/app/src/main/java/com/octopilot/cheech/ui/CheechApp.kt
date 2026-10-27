package com.octopilot.cheech.ui

import android.net.Uri
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.material3.Surface
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.ui.Modifier
import com.octopilot.cheech.model.Screen
import com.octopilot.cheech.model.SessionModel
import com.octopilot.cheech.ui.game.GameScreen

@Composable
fun CheechApp(
    model: SessionModel,
    inviteUri: Uri?,
    onInviteConsumed: () -> Unit,
) {
    LaunchedEffect(inviteUri) {
        if (inviteUri != null) {
            model.handleInvite(inviteUri)
            onInviteConsumed()
        }
    }

    Surface(Modifier.fillMaxSize()) {
        when (model.screen) {
            Screen.Setup -> SetupScreen(model)
            Screen.Game -> GameScreen(model)
        }
    }
}
