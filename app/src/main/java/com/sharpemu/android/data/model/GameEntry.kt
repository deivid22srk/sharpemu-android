package com.sharpemu.android.data.model

data class GameEntry(
    val name: String,
    val titleId: String? = null,
    val version: String? = null,
    val path: String,
    val sizeBytes: Long = 0,
    val coverPath: String? = null,
    val backgroundPath: String? = null
) {
    val initials: String
        get() = name.take(2).uppercase()

    val sizeText: String
        get() = when {
            sizeBytes >= 1_073_741_824 -> "%.1f GB".format(sizeBytes / 1_073_741_824.0)
            sizeBytes >= 1_048_576 -> "%.1f MB".format(sizeBytes / 1_048_576.0)
            sizeBytes >= 1024 -> "%.1f KB".format(sizeBytes / 1024.0)
            else -> "$sizeBytes B"
        }

    val hasTitleId: Boolean get() = !titleId.isNullOrBlank()
    val hasVersion: Boolean get() = !version.isNullOrBlank()
    val hasCover: Boolean get() = !coverPath.isNullOrBlank()
}

data class EmulatorSettings(
    val cpuEngine: String = "Native",
    val strictDynlibResolution: Boolean = false,
    val logLevel: String = "Info",
    val importTraceLimit: Int = 0,
    val logToFile: Boolean = false,
    val logFilePath: String = "",
    val overrideLogFile: Boolean = false,
    val language: String = "en",
    val playTitleMusic: Boolean = true,
    val discordPresence: Boolean = false,
    val autoUpdate: Boolean = true,
    val gameFolders: MutableList<String> = mutableListOf(),
    val excludedGames: MutableList<String> = mutableListOf()
)

enum class EmulatorState {
    IDLE, LOADING, RUNNING, STOPPING
}
