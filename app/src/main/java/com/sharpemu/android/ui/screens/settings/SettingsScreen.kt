package com.sharpemu.android.ui.screens.settings

import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.ExposedDropdownMenuBox
import androidx.compose.material3.ExposedDropdownMenuDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Switch
import androidx.compose.material3.Text
import androidx.compose.material3.TopAppBar
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import com.sharpemu.android.data.model.EmulatorSettings
import com.sharpemu.android.ui.screens.SharpEmuViewModel

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun SettingsScreen(viewModel: SharpEmuViewModel) {
    val settings by viewModel.settings.collectAsState()

    Scaffold(
        topBar = {
            TopAppBar(title = { Text("Options") })
        }
    ) { padding ->
        Column(
            modifier = Modifier
                .fillMaxSize()
                .padding(padding)
                .verticalScroll(rememberScrollState())
                .padding(16.dp)
        ) {
            SettingsCard(title = "EMULATION") {
                SettingsDropdown(
                    label = "CPU engine",
                    description = "Execution engine used to run game code.",
                    options = listOf("Native"),
                    selected = settings.cpuEngine,
                    onSelect = { viewModel.updateSettings(settings.copy(cpuEngine = it)) }
                )
                Spacer(modifier = Modifier.height(12.dp))
                SettingsToggle(
                    label = "Strict dynlib resolution",
                    description = "Fail the launch when an imported symbol cannot be resolved.",
                    checked = settings.strictDynlibResolution,
                    onCheckedChange = {
                        viewModel.updateSettings(settings.copy(strictDynlibResolution = it))
                    }
                )
            }

            Spacer(modifier = Modifier.height(16.dp))

            SettingsCard(title = "LOGGING") {
                SettingsDropdown(
                    label = "Log level",
                    description = "Verbosity of the emulator console output.",
                    options = listOf("Trace", "Debug", "Info", "Warning", "Error", "Critical"),
                    selected = settings.logLevel,
                    onSelect = { viewModel.updateSettings(settings.copy(logLevel = it)) }
                )
                Spacer(modifier = Modifier.height(12.dp))
                SettingsToggle(
                    label = "Log to file",
                    description = "Mirror emulator output to a log file.",
                    checked = settings.logToFile,
                    onCheckedChange = {
                        viewModel.updateSettings(settings.copy(logToFile = it))
                    }
                )
            }

            Spacer(modifier = Modifier.height(16.dp))

            SettingsCard(title = "LAUNCHER") {
                SettingsDropdown(
                    label = "Emulator language",
                    description = "Language used throughout the launcher.",
                    options = listOf("en", "pt", "es", "fr", "de", "ja", "ko", "ru"),
                    selected = settings.language,
                    onSelect = { viewModel.updateSettings(settings.copy(language = it)) }
                )
                Spacer(modifier = Modifier.height(12.dp))
                SettingsToggle(
                    label = "Title music",
                    description = "Loop the selected game's preview music in the library.",
                    checked = settings.playTitleMusic,
                    onCheckedChange = {
                        viewModel.updateSettings(settings.copy(playTitleMusic = it))
                    }
                )
                Spacer(modifier = Modifier.height(12.dp))
                SettingsToggle(
                    label = "Check for updates on startup",
                    description = "Checks GitHub without delaying startup.",
                    checked = settings.autoUpdate,
                    onCheckedChange = {
                        viewModel.updateSettings(settings.copy(autoUpdate = it))
                    }
                )
            }

            Spacer(modifier = Modifier.height(16.dp))

            SettingsCard(title = "ABOUT") {
                Text(
                    "SharpEmu Android v0.1.0",
                    style = MaterialTheme.typography.bodyLarge
                )
                Spacer(modifier = Modifier.height(4.dp))
                Text(
                    "PS4/PS5 emulator — Android frontend",
                    style = MaterialTheme.typography.bodyMedium,
                    color = MaterialTheme.colorScheme.onSurfaceVariant
                )
            }
        }
    }
}

@Composable
private fun SettingsCard(title: String, content: @Composable () -> Unit) {
    Card(
        modifier = Modifier.fillMaxWidth(),
        colors = CardDefaults.cardColors(
            containerColor = MaterialTheme.colorScheme.surfaceVariant.copy(alpha = 0.4f)
        ),
        shape = MaterialTheme.shapes.large
    ) {
        Column(modifier = Modifier.padding(16.dp)) {
            Text(
                title,
                style = MaterialTheme.typography.labelSmall,
                color = MaterialTheme.colorScheme.primary
            )
            Spacer(modifier = Modifier.height(12.dp))
            content()
        }
    }
}

@Composable
private fun SettingsToggle(
    label: String,
    description: String,
    checked: Boolean,
    onCheckedChange: (Boolean) -> Unit
) {
    Row(
        modifier = Modifier.fillMaxWidth(),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Column(modifier = Modifier.weight(1f)) {
            Text(label, style = MaterialTheme.typography.bodyLarge)
            Text(
                description,
                style = MaterialTheme.typography.bodySmall,
                color = MaterialTheme.colorScheme.onSurfaceVariant
            )
        }
        Spacer(modifier = Modifier.width(12.dp))
        Switch(checked = checked, onCheckedChange = onCheckedChange)
    }
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
private fun SettingsDropdown(
    label: String,
    description: String,
    options: List<String>,
    selected: String,
    onSelect: (String) -> Unit
) {
    var expanded by remember { mutableStateOf(false) }

    Column {
        Text(label, style = MaterialTheme.typography.bodyLarge)
        Text(
            description,
            style = MaterialTheme.typography.bodySmall,
            color = MaterialTheme.colorScheme.onSurfaceVariant
        )
        Spacer(modifier = Modifier.height(8.dp))
        ExposedDropdownMenuBox(
            expanded = expanded,
            onExpandedChange = { expanded = it }
        ) {
            OutlinedTextField(
                value = selected,
                onValueChange = {},
                readOnly = true,
                trailingIcon = { ExposedDropdownMenuDefaults.TrailingIcon(expanded = expanded) },
                modifier = Modifier
                    .menuAnchor()
                    .fillMaxWidth()
            )
            ExposedDropdownMenu(
                expanded = expanded,
                onDismissRequest = { expanded = false }
            ) {
                options.forEach { option ->
                    DropdownMenuItem(
                        text = { Text(option) },
                        onClick = {
                            onSelect(option)
                            expanded = false
                        }
                    )
                }
            }
        }
    }
}
