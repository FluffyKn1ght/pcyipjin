/**
 * @file emulator.h
 * @brief Represents an active pcyipjin emulator (emulates a PC Engine/TurboGrafx-16 system)
 */

#ifndef _PCYIPJIN_EMULATOR_H
#define _PCYIPJIN_EMULATOR_H

#include "memory.h"
#include "pce/cpu.h"
#include "pce/vce.h"
#include "pce/vdc.h"

#define MASTER_CLOCK 21'147'727
#define CPU_CLOCKDIV_HIGH 3
#define CPU_CLOCKDIV_LOW 12
#define TIMER_CLOCKDIV 3072

/**
 * @brief Represents a pcyipjin emulator instance
 */
typedef struct {
    CPU* cpu;    /**< The CPU this emulator uses */
    Memory* mem; /**< The Memory bus this emulator uses */
    VDC* vdc;    /**< The VDC chip this emulator uses */
    VCE* vce;    /**< The VCE chip this emulator uses */

    u64 cycles; /**< Master clock cycle counter */
} Emulator;

/**
 * @brief Creates a new emulator instance
 *
 * @return The created Emulator instance
 */
Emulator* emu_create();

/**
 * @brief Destroys and frees the provided emulator
 *
 * @param emu The Emulator to destroy
 */
void emu_destroy(Emulator* emu);

/**
 * @brief Steps the provided emulator forward by 1 CPU instruction
 *
 * @note emu_reset() should be called
 *
 * @param emu The Emulator to step forward
 */
void emu_step(Emulator* emu);

/**
 * @brief Resets the provided emulator
 *
 * @param emu The Emulator to reset
 * @param hard Whether to perform a soft-reset (pressing the button on the console) or a hard-reset
 * (power cycle)
 */
void emu_reset(Emulator* emu, bool hard);

#endif
