package com.sharpemu.android.ui.screens

import android.app.Application
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import com.sharpemu.android.data.model.EmulatorSettings
import com.sharpemu.android.data.model.EmulatorState
import com.sharpemu.android.data.model.GameEntry
import com.sharpemu.android.data.repository.GameRepository
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch

class SharpEmuViewModel(application: Application) : AndroidViewModel(application) {

    private val repository = GameRepository(application)

    private val _games = MutableStateFlow<List<GameEntry>>(emptyList())
    val games: StateFlow<List<GameEntry>> = _games.asStateFlow()

    private val _filteredGames = MutableStateFlow<List<GameEntry>>(emptyList())
    val filteredGames: StateFlow<List<GameEntry>> = _filteredGames.asStateFlow()

    private val _selectedGame = MutableStateFlow<GameEntry?>(null)
    val selectedGame: StateFlow<GameEntry?> = _selectedGame.asStateFlow()

    private val _emulatorState = MutableStateFlow(EmulatorState.IDLE)
    val emulatorState: StateFlow<EmulatorState> = _emulatorState.asStateFlow()

    private val _settings = MutableStateFlow(EmulatorSettings())
    val settings: StateFlow<EmulatorSettings> = _settings.asStateFlow()

    private val _searchQuery = MutableStateFlow("")
    val searchQuery: StateFlow<String> = _searchQuery.asStateFlow()

    private val _isLoading = MutableStateFlow(false)
    val isLoading: StateFlow<Boolean> = _isLoading.asStateFlow()

    private val _consoleLines = MutableStateFlow<List<String>>(emptyList())
    val consoleLines: StateFlow<List<String>> = _consoleLines.asStateFlow()

    fun setSearchQuery(query: String) {
        _searchQuery.value = query
        filterGames()
    }

    fun selectGame(game: GameEntry?) {
        _selectedGame.value = game
    }

    fun addFolder(path: String) {
        val current = _settings.value
        if (!current.gameFolders.contains(path)) {
            _settings.value = current.copy(
                gameFolders = (current.gameFolders + path).toMutableList()
            )
            scanLibrary()
        }
    }

    fun removeGame(game: GameEntry) {
        val current = _settings.value
        _settings.value = current.copy(
            excludedGames = (current.excludedGames + game.path).toMutableList()
        )
        _games.value = _games.value.filter { it.path != game.path }
        if (_selectedGame.value?.path == game.path) {
            _selectedGame.value = null
        }
        filterGames()
    }

    fun scanLibrary() {
        viewModelScope.launch {
            _isLoading.value = true
            val result = repository.scanFolders(
                _settings.value.gameFolders,
                _settings.value.excludedGames.toSet()
            )
            _games.value = result
            filterGames()
            _isLoading.value = false
        }
    }

    fun launchGame(game: GameEntry) {
        _selectedGame.value = game
        _emulatorState.value = EmulatorState.LOADING
        appendConsole("[INFO] Launching: ${game.name}")
        appendConsole("[INFO] Path: ${game.path}")
        appendConsole("[INFO] CPU Engine: ${_settings.value.cpuEngine}")
        appendConsole("[INFO] Log Level: ${_settings.value.logLevel}")
        appendConsole("[WARNING] Emulation core not yet available on Android.")
        appendConsole("[INFO] See project roadmap for Android execution plans.")
    }

    fun stopGame() {
        if (_emulatorState.value == EmulatorState.RUNNING || _emulatorState.value == EmulatorState.LOADING) {
            _emulatorState.value = EmulatorState.STOPPING
            appendConsole("[INFO] Stopping emulation session...")
            _emulatorState.value = EmulatorState.IDLE
            appendConsole("[INFO] Session stopped.")
        }
    }

    fun appendConsole(line: String) {
        _consoleLines.value = _consoleLines.value + line
    }

    fun clearConsole() {
        _consoleLines.value = emptyList()
    }

    fun updateSettings(settings: EmulatorSettings) {
        _settings.value = settings
    }

    private fun filterGames() {
        val query = _searchQuery.value.trim()
        _filteredGames.value = if (query.isEmpty()) {
            _games.value
        } else {
            _games.value.filter {
                it.name.contains(query, ignoreCase = true) ||
                    it.path.contains(query, ignoreCase = true) ||
                    (it.titleId?.contains(query, ignoreCase = true) == true)
            }
        }
    }
}
