/**
 * @file emulator.h
 * @brief Contains the core structs and functions that "glue" the emulator together
 */

#ifndef _PCYIPJIN_EMULATOR_H
#define _PCYIPJIN_EMULATOR_H

#define MASTER_CLOCK 21'147'727
#define CPU_CLOCKDIV_HIGH 3
#define CPU_CLOCKDIV_LOW 12
#define TIMER_CLOCKDIV 3072
#define VCE_CLOCKDIV_10MHZ 2
#define VCE_CLOCKDIV_7MHZ 3
#define VCE_CLOCKDIV_5MHZ 4

/**
 * @brief Represents a pcyipjin emulator, with all of its state
 */
#include "pce/cpu.h"
#include "pce/memory.h"
typedef struct {
    CPU* cpu;
    Memory* mem;

    u16 vce_clock_modulo;
    u16 vce_clock;
    u16 timer_clock;
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
