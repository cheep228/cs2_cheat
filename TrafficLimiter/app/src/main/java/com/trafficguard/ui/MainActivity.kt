package com.trafficguard.ui

import android.os.Build
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.BackHandler
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.isSystemInDarkTheme
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.darkColorScheme
import androidx.compose.material3.dynamicDarkColorScheme
import androidx.compose.material3.dynamicLightColorScheme
import androidx.compose.material3.lightColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.runtime.mutableStateListOf
import androidx.compose.runtime.remember
import androidx.compose.ui.platform.LocalContext
import com.trafficguard.data.SettingsRepo
import com.trafficguard.service.RootShell
import com.trafficguard.service.TrafficService

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        if (SettingsRepo.settings.value.monitorEnabled) {
            runCatching { TrafficService.start(this) }
        }
        probeRootOnce()
        setContent { AppTheme { AppRoot() } }
    }

    /** First launch: if the phone is rooted, block through root by default. */
    private fun probeRootOnce() {
        if (SettingsRepo.settings.value.rootProbed) return
        Thread {
            val ok = RootShell.available()
            SettingsRepo.update { it.copy(rootProbed = true, useRoot = it.useRoot || ok) }
        }.start()
    }
}

sealed interface Screen {
    data object Home : Screen
    data class EditProfile(val id: String) : Screen
    data class Apps(val id: String) : Screen
}

@Composable
fun AppRoot() {
    val stack = remember { mutableStateListOf<Screen>(Screen.Home) }
    val back: () -> Unit = { if (stack.size > 1) stack.removeAt(stack.lastIndex) }
    BackHandler(enabled = stack.size > 1, onBack = back)
    when (val top = stack.last()) {
        Screen.Home -> HomeScreen(onOpenProfile = { stack.add(Screen.EditProfile(it)) })
        is Screen.EditProfile -> ProfileScreen(top.id, onBack = back, onApps = { stack.add(Screen.Apps(top.id)) })
        is Screen.Apps -> AppsScreen(top.id, onBack = back)
    }
}

@Composable
fun AppTheme(content: @Composable () -> Unit) {
    val dark = isSystemInDarkTheme()
    val ctx = LocalContext.current
    val scheme = when {
        Build.VERSION.SDK_INT >= 31 -> if (dark) dynamicDarkColorScheme(ctx) else dynamicLightColorScheme(ctx)
        dark -> darkColorScheme()
        else -> lightColorScheme()
    }
    MaterialTheme(colorScheme = scheme, content = content)
}
