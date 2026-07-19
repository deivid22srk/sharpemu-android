package com.sharpemu.android.emulation

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.Service
import android.content.Intent
import android.os.Binder
import android.os.Build
import android.os.IBinder
import androidx.core.app.NotificationCompat
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch

class EmulatorService : Service() {

    private val binder = EmulatorBinder()
    private val scope = CoroutineScope(SupervisorJob() + Dispatchers.IO)
    private val emulator = FexEmulator()

    private val _state = MutableStateFlow(EmulatorState.IDLE)
    val state: StateFlow<EmulatorState> = _state.asStateFlow()

    private val _log = MutableStateFlow("")
    val log: StateFlow<String> = _log.asStateFlow()

    inner class EmulatorBinder : Binder() {
        fun getService(): EmulatorService = this@EmulatorService
    }

    override fun onBind(intent: Intent?): IBinder = binder

    override fun onCreate() {
        super.onCreate()
        createNotificationChannel()
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        when (intent?.action) {
            ACTION_START -> {
                val elfPath = intent.getStringExtra(EXTRA_ELF_PATH) ?: return START_NOT_STICKY
                startForeground(NOTIFICATION_ID, createNotification("Starting emulation..."))
                startEmulation(elfPath)
            }
            ACTION_STOP -> stopEmulation()
        }
        return START_STICKY
    }

    private fun startEmulation(elfPath: String) {
        scope.launch {
            _state.value = EmulatorState.LOADING
            appendLog("Initializing FEX-EMU core...")

            if (!emulator.create()) {
                appendLog("ERROR: Failed to create FEX emulator instance")
                _state.value = EmulatorState.ERROR
                return@launch
            }

            appendLog("Loading ELF: $elfPath")
            if (!emulator.loadElf(elfPath)) {
                appendLog("ERROR: Failed to load ELF binary")
                emulator.destroy()
                _state.value = EmulatorState.ERROR
                return@launch
            }

            appendLog("Starting guest execution...")
            if (!emulator.start()) {
                appendLog("ERROR: Failed to start execution")
                emulator.destroy()
                _state.value = EmulatorState.ERROR
                return@launch
            }

            _state.value = EmulatorState.RUNNING
            appendLog("Emulation running")
            updateNotification("Emulation running")
        }
    }

    private fun stopEmulation() {
        scope.launch {
            appendLog("Stopping emulation...")
            emulator.stop()
            emulator.destroy()
            _state.value = EmulatorState.IDLE
            appendLog("Emulation stopped")
            stopForeground(STOP_FOREGROUND_REMOVE)
            stopSelf()
        }
    }

    private fun appendLog(message: String) {
        _log.value += "[${System.currentTimeMillis()}] $message\n"
    }

    private fun createNotificationChannel() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            val channel = NotificationChannel(
                CHANNEL_ID, "Emulation", NotificationManager.IMPORTANCE_LOW
            )
            getSystemService(NotificationManager::class.java)
                .createNotificationChannel(channel)
        }
    }

    private fun createNotification(text: String): Notification {
        return NotificationCompat.Builder(this, CHANNEL_ID)
            .setContentTitle("SharpEmu")
            .setContentText(text)
            .setSmallIcon(android.R.drawable.ic_media_play)
            .setOngoing(true)
            .build()
    }

    private fun updateNotification(text: String) {
        val nm = getSystemService(NotificationManager::class.java)
        nm.notify(NOTIFICATION_ID, createNotification(text))
    }

    override fun onDestroy() {
        emulator.stop()
        emulator.destroy()
        super.onDestroy()
    }

    companion object {
        const val ACTION_START = "com.sharpemu.android.ACTION_START"
        const val ACTION_STOP = "com.sharpemu.android.ACTION_STOP"
        const val EXTRA_ELF_PATH = "elf_path"
        private const val CHANNEL_ID = "sharpemu_emulation"
        private const val NOTIFICATION_ID = 1001
    }
}

enum class EmulatorState {
    IDLE, LOADING, RUNNING, PAUSED, ERROR
}
