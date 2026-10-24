/**
 * @file emulator.h
 * @brief Represents an active pcyipjin emulator (emulates a PC Engine/TurboGrafx-16 system)
 */

#ifndef _PCYIPJIN_EMULATOR_H
#define _PCYIPJIN_EMULATOR_H

#include "memory.h"
#include "pce/cpu.h"
#include "pce/graphics.h"

#define MASTER_CLOCK 21'147'727
#define CPU_CLOCKDIV_HIGH 3
#define CPU_CLOCKDIV_LOW 12
#define TIMER_CLOCKDIV 3072

/**
 * @brief Describes the features of an emulated console
 *
 * @warning Console features are not changeable mid-emulation, the PCEEmulator instance has to be
 * completely recreated!!
 */
typedef struct {
    bool supergrafx : 1; /**< Console is a SuperGrafx unit (extra memory and graphics power) */
    bool cdrom : 1;      /**< Console has a CD-ROM2 expansion attached */
} PCEConsoleFeatures;

/**
 * @brief Represents a pcyipjin emulator instance
 */
typedef struct {
    PCEConsoleFeatures features; /**< The features of the emulated console */

    HuC6280* cpu;   /**< The CPU this emulator uses */
    PCEMemory* mem; /**< The Memory bus this emulator uses */

    PCEGraphics* gfx; /**< The graphics pipeline this emulator uses */

    u64 cycles; /**< Master clock cycle counter */
} PCEEmulator;

/**
 * @brief Creates a new emulator instance
 *
 * @param features The features the emulated console should have
 *
 * @return The created PCEEmulator instance
 */
PCEEmulator* pce_emu_create(PCEConsoleFeatures features);

/**
 * @brief Destroys and frees the provided emulator
 *
 * @param emu The PCEEmulator to destroy
 */
void pce_emu_destroy(PCEEmulator* emu);

/**
 * @brief Steps the provided emulator forward by 1 CPU instruction
 *
 * @note emu_reset() should be called before this
 *

 * @param emu The PCEEmulator to step forward
 */
void pce_emu_step(PCEEmulator* emu);

/**
 * @brief Resets the provided emulator
 *
 * @param emu The PCEEmulator to reset
 * @param hard Whether to perform a soft-reset (pressing the button on the console) or a hard-reset
 * (power cycle)
 */
void pce_emu_reset(PCEEmulator* emu, bool hard);

#endif
