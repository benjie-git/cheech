package com.octopilot.cheech.ui.theme

import android.app.Activity
import androidx.compose.foundation.isSystemInDarkTheme
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.darkColorScheme
import androidx.compose.material3.lightColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.runtime.SideEffect
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.toArgb
import androidx.compose.ui.platform.LocalView
import androidx.core.view.WindowCompat

private val Green = Color(0xFF2E7D32)
private val GreenLight = Color(0xFF60AD5E)
private val Brown = Color(0xFF8C663D)

private val LightColors = lightColorScheme(
    primary = Green,
    onPrimary = Color.White,
    primaryContainer = Color(0xFFB7E4B9),
    onPrimaryContainer = Color(0xFF0A2E0C),
    secondary = Brown,
    onSecondary = Color.White,
    background = Color(0xFFF7F7F2),
    surface = Color(0xFFFFFBFE),
    surfaceVariant = Color(0xFFE7E0D8),
)

private val DarkColors = darkColorScheme(
    primary = GreenLight,
    onPrimary = Color(0xFF00390A),
    primaryContainer = Color(0xFF14521A),
    onPrimaryContainer = Color(0xFFB7E4B9),
    secondary = Color(0xFFD8B68C),
    background = Color(0xFF121412),
    surface = Color(0xFF1A1C19),
    surfaceVariant = Color(0xFF3F3A34),
)

@Composable
fun CheechTheme(
    darkTheme: Boolean = isSystemInDarkTheme(),
    content: @Composable () -> Unit,
) {
    val colors = if (darkTheme) DarkColors else LightColors
    val view = LocalView.current
    if (!view.isInEditMode) {
        SideEffect {
            val window = (view.context as Activity).window
            window.statusBarColor = Color.Transparent.toArgb()
            window.navigationBarColor = Color.Transparent.toArgb()
            WindowCompat.getInsetsController(window, view).isAppearanceLightStatusBars = !darkTheme
            WindowCompat.getInsetsController(window, view).isAppearanceLightNavigationBars = !darkTheme
        }
    }
    MaterialTheme(colorScheme = colors, content = content)
}
