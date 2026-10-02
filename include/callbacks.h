/**
 * @file callbacks.h
 * @brief Declares an interface for emulator module cross-communication
 */

#ifndef _PCYIPJIN_CALLBACKS_H
#define _PCYIPJIN_CALLBACKS_H

/**
 * @brief Contains callback function pointers and data for communicating with the Emulator struct
 */
typedef struct {
    void* arg0; /**< Pointer to the Emulator struct (Emulator*) */

    void (*cpu_sync)(
        void* arg0); /**< Syncs other modules with the CPU (emulator.c::_emu_cpusync) */
    void (*vdc_write)(void* arg0, u8 addr, u8 value); /**< Writes data to the VDC (HuC6270) */
} EmuCallbacks;

#endif
