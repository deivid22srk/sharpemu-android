package com.sharpemu.android.data.repository

import android.content.Context
import com.sharpemu.android.data.model.GameEntry
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import org.json.JSONObject
import java.io.File

class GameRepository(private val context: Context) {

    suspend fun scanFolders(folders: List<String>, excluded: Set<String>): List<GameEntry> =
        withContext(Dispatchers.IO) {
            val games = mutableListOf<GameEntry>()
            val seen = mutableSetOf<String>()

            for (folder in folders) {
                val dir = File(folder)
                if (!dir.exists() || !dir.isDirectory) continue

                dir.walkTopDown()
                    .maxDepth(8)
                    .filter { it.name == "eboot.bin" }
                    .forEach { eboot ->
                        val fullPath = eboot.absolutePath
                        if (!seen.add(fullPath) || excluded.contains(fullPath)) return@forEach

                        val (title, titleId, version) = readParamJson(eboot)
                        games.add(
                            GameEntry(
                                name = title ?: eboot.parentFile?.name ?: eboot.name,
                                titleId = titleId,
                                version = version,
                                path = fullPath,
                                sizeBytes = computeInstallSize(eboot),
                                coverPath = findCover(eboot),
                                backgroundPath = findBackground(eboot)
                            )
                        )
                    }
            }

            games.sortedBy { it.name.lowercase() }
        }

    private fun readParamJson(eboot: File): Triple<String?, String?, String?> {
        val paramFile = File(eboot.parentFile, "sce_sys/param.json")
        if (!paramFile.exists()) return Triple(null, null, null)

        return try {
            val json = JSONObject(paramFile.readText())
            val titleId = json.optString("titleId", null)
            val version = json.optString("contentVersion", null)
                ?: json.optString("masterVersion", null)

            var title: String? = null
            val localized = json.optJSONObject("localizedParameters")
            if (localized != null) {
                val defaultLang = localized.optString("defaultLanguage", "")
                val block = localized.optJSONObject(defaultLang)
                title = block?.optString("titleName", null)

                if (title == null) {
                    val keys = localized.keys()
                    while (keys.hasNext() && title == null) {
                        val obj = localized.optJSONObject(keys.next())
                        title = obj?.optString("titleName", null)
                    }
                }
            }

            Triple(title, titleId, version)
        } catch (_: Exception) {
            Triple(null, null, null)
        }
    }

    private fun findCover(eboot: File): String? {
        val sceSys = File(eboot.parentFile, "sce_sys")
        return listOf("icon0.png", "pic0.png")
            .map { File(sceSys, it) }
            .firstOrNull { it.exists() }
            ?.absolutePath
    }

    private fun findBackground(eboot: File): String? {
        val sceSys = File(eboot.parentFile, "sce_sys")
        return listOf("pic0.png", "pic1.png")
            .map { File(sceSys, it) }
            .firstOrNull { it.exists() }
            ?.absolutePath
    }

    private fun computeInstallSize(eboot: File): Long {
        val dir = eboot.parentFile ?: return 0
        return try {
            dir.walkTopDown().filter { it.isFile }.sumOf { it.length() }
        } catch (_: Exception) {
            0L
        }
    }
}
