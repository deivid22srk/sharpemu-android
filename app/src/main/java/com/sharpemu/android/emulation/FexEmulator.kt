package com.sharpemu.android.emulation

class FexEmulator {
    private var nativeHandle: Long = 0

    val isInitialized: Boolean get() = nativeHandle != 0L

    fun create(): Boolean {
        nativeHandle = nativeCreate()
        return nativeHandle != 0L
    }

    fun destroy() {
        if (nativeHandle != 0L) {
            nativeDestroy(nativeHandle)
            nativeHandle = 0
        }
    }

    fun loadElf(path: String): Boolean {
        if (nativeHandle == 0L) return false
        return nativeLoadElf(nativeHandle, path)
    }

    fun start(): Boolean {
        if (nativeHandle == 0L) return false
        nativeSetHleDispatch(nativeHandle)
        return nativeStart(nativeHandle)
    }

    fun stop() {
        if (nativeHandle != 0L) nativeStop(nativeHandle)
    }

    val isRunning: Boolean
        get() = nativeHandle != 0L && nativeIsRunning(nativeHandle)

    fun mapMemory(address: Long, size: Long, executable: Boolean, writable: Boolean): Long {
        if (nativeHandle == 0L) return 0
        return nativeMapMemory(nativeHandle, address, size, executable, writable)
    }

    fun writeMemory(address: Long, data: ByteArray): Boolean {
        if (nativeHandle == 0L) return false
        return nativeWriteMemory(nativeHandle, address, data)
    }

    @Suppress("unused")
    fun handleSyscall(syscallNum: Int, args: LongArray): Long {
        return when (syscallNum) {
            0 -> 0L // sys_exit
            1 -> args[2] // sys_write
            20 -> 1L // sys_getpid
            602 -> 1L // sys_is_development_mode
            617 -> 4096L // sys_get_phys_page_size
            else -> 0L
        }
    }

    private external fun nativeCreate(): Long
    private external fun nativeDestroy(handle: Long)
    private external fun nativeLoadElf(handle: Long, path: String): Boolean
    private external fun nativeStart(handle: Long): Boolean
    private external fun nativeStop(handle: Long)
    private external fun nativeIsRunning(handle: Long): Boolean
    private external fun nativeMapMemory(handle: Long, addr: Long, size: Long, exec: Boolean, write: Boolean): Long
    private external fun nativeWriteMemory(handle: Long, addr: Long, data: ByteArray): Boolean
    private external fun nativeSetHleDispatch(handle: Long)

    companion object {
        init {
            System.loadLibrary("sharpemu_native")
        }
    }
}
