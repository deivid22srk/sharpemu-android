package com.sharpemu.android.ui.screens

import androidx.compose.animation.AnimatedVisibility
import androidx.compose.animation.fadeIn
import androidx.compose.animation.fadeOut
import androidx.compose.animation.slideInVertically
import androidx.compose.animation.slideOutVertically
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Settings
import androidx.compose.material.icons.filled.SportsEsports
import androidx.compose.material3.Icon
import androidx.compose.material3.NavigationBar
import androidx.compose.material3.NavigationBarItem
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.lifecycle.viewmodel.compose.viewModel
import com.sharpemu.android.ui.components.LaunchBar
import com.sharpemu.android.ui.screens.library.LibraryScreen
import com.sharpemu.android.ui.screens.settings.SettingsScreen

@Composable
fun SharpEmuApp(viewModel: SharpEmuViewModel = viewModel()) {
    var selectedTab by remember { mutableIntStateOf(0) }
    val emulatorState by viewModel.emulatorState.collectAsState()
    val selectedGame by viewModel.selectedGame.collectAsState()

    Scaffold(
        bottomBar = {
            Column {
                AnimatedVisibility(
                    visible = selectedGame != null,
                    enter = slideInVertically(initialOffsetY = { it }) + fadeIn(),
                    exit = slideOutVertically(targetOffsetY = { it }) + fadeOut()
                ) {
                    LaunchBar(
                        game = selectedGame,
                        emulatorState = emulatorState,
                        onLaunch = { selectedGame?.let { viewModel.launchGame(it) } },
                        onStop = { viewModel.stopGame() }
                    )
                }
                NavigationBar {
                    NavigationBarItem(
                        selected = selectedTab == 0,
                        onClick = { selectedTab = 0 },
                        icon = { Icon(Icons.Default.SportsEsports, contentDescription = "Library") },
                        label = { Text("Library") }
                    )
                    NavigationBarItem(
                        selected = selectedTab == 1,
                        onClick = { selectedTab = 1 },
                        icon = { Icon(Icons.Default.Settings, contentDescription = "Options") },
                        label = { Text("Options") }
                    )
                }
            }
        }
    ) { padding ->
        Column(
            modifier = Modifier
                .fillMaxSize()
                .padding(padding)
        ) {
            when (selectedTab) {
                0 -> LibraryScreen(viewModel = viewModel)
                1 -> SettingsScreen(viewModel = viewModel)
            }
        }
    }
}
