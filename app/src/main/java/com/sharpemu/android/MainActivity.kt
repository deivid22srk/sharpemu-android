package com.sharpemu.android

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import com.sharpemu.android.ui.theme.SharpEmuTheme
import com.sharpemu.android.ui.screens.SharpEmuApp

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContent {
            SharpEmuTheme {
                SharpEmuApp()
            }
        }
    }
}
