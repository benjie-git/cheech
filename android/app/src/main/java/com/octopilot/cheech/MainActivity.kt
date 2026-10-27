package com.octopilot.cheech

import android.content.Intent
import android.net.Uri
import android.os.Bundle
import android.view.KeyEvent
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.activity.viewModels
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.lifecycle.lifecycleScope
import com.octopilot.cheech.bridge.CheechSession
import com.octopilot.cheech.model.Screen
import com.octopilot.cheech.model.SessionModel
import com.octopilot.cheech.ui.CheechApp
import com.octopilot.cheech.ui.theme.CheechTheme
import java.io.File

class MainActivity : ComponentActivity() {
    private val model: SessionModel by viewModels()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        prepareWebAssets()
        setContent {
            CheechTheme {
                var inviteUri by remember { mutableStateOf<Uri?>(intent?.data) }
                CheechApp(model = model, inviteUri = inviteUri, onInviteConsumed = { inviteUri = null })
            }
        }
    }

    override fun onNewIntent(intent: Intent) {
        super.onNewIntent(intent)
        setIntent(intent)
        // Recreate is unnecessary; the Compose state observes this through the
        // activity's intent on the next launch.  Forward the URI explicitly.
        intent.data?.let { uri ->
            model.handleInvite(uri)
        }
    }

    override fun onStop() {
        model.persistLocalGame()
        super.onStop()
    }

    override fun dispatchKeyEvent(event: KeyEvent): Boolean {
        if (super.dispatchKeyEvent(event)) return true
        if (event.keyCode == KeyEvent.KEYCODE_ESCAPE &&
            model.screen == Screen.Game && !model.chatOpen
        ) {
            // Escape cancels the in-progress move path. Consume it even when
            // the focused UI did not so Android cannot re-dispatch it as Back
            // and close the game screen.
            if (event.action == KeyEvent.ACTION_DOWN) {
                val snapshot = model.snapshot
                if (snapshot != null && model.canControl(snapshot) &&
                    snapshot.selected.isNotEmpty()
                ) {
                    model.clearSelection()
                }
            }
            return true
        }
        return false
    }

    private fun prepareWebAssets() {
        val target = File(filesDir, "web")
        target.mkdirs()
        copyAsset("index.html", target)
        copyAsset("favicon.svg", target)
        copyAsset("cheech.png", target)
        copyAsset("css/cheech.css", target)
        copyAsset("js/app.js", target)
        copyAsset("js/board.js", target)
        copyAsset("js/protocol.js", target)
        CheechSession.setWebRoot(target.absolutePath)
    }

    private fun copyAsset(name: String, root: File) {
        val out = File(root, name)
        out.parentFile?.mkdirs()
        try {
            assets.open("web/$name").use { input ->
                out.outputStream().use { output -> input.copyTo(output) }
            }
        } catch (_: Exception) {
        }
    }
}
