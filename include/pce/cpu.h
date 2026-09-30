/**
 * @file pce/cpu.h
 * @brief Emulates a HuC6280 (MOS6502 variant) CPU
 */

#ifndef _PCYIPJIN_PCE_CPU_H
#define _PCYIPJIN_PCE_CPU_H

#include "pce/memory.h"
#include "stdbool.h"

typedef struct {
    bool c : 1; /**< Carry flag (unsigned overflow/underflow) */
    bool z : 1; /**< Zero flag (result == 0) */
    bool i : 1; /**< Interrupt Disable (no timer/IRQs if 1) */
    bool d : 1; /**< Decimal flag (ADC/SBC does decimal adjustment, +1 cycle) */
    bool b : 1; /**< Break flag (0 if IRQ2 occured, 1 if BRK occured) */
    bool t : 1; /**< Memory Operation flag (ADC/AND/OR/EOR write to zeropage offset X if 1, to A if
                   0) */
    bool v : 1; /**< Overflow flag (signed overflow/underflow) */
    bool n : 1; /**< Negative flag (set if bit 7 of ALU result is 1) */
} CPUStatus;

/**
 * @brief Represents the state of an emulated HuC6280 CPU
 */
typedef struct {
    u8 acc; /**< Accumulator register (ACC) */
    u8 x;   /**< X register */
    u8 y;   /**< Y register */
    u8 sp;  /**< Stack pointer */
    u16 pc; /**< Program counter */

    union {
        CPUStatus status; /**< Status register, as a CPUStatus bitfield */
        u8 p;             /**< Status register, as a byte */
    };

    u8 sh; /**< Block transfer source high (SH). Paired with X to get a 16-bit value */
    u8 dh; /**< Block transfer destination high (DH). Paired with Y to get a 16-bit value */
    u8 lh; /**< Block transfer length high (LH). Paired with ACC to get a 16-bit value */

    // TODO: Interrupt stuffs
    // TODO: Timer
    // TODO: I/O (K and O ports)

    u8 mpr[8]; /**< Mapping registers (MPR0-MPR7) */

    bool high_speed; /**< Whether the CPU is running at 7MHz high speed */
} CPU;

/**
 * @brief Resets the CPU state.
 *
 * @param cpu The CPU to reset
 * @param mem The memory bus the CPU is connected to
 * @param sync_func The function to call every time the system needs to be advanced by 1 CPU clock
 * tick
 * @param sync_arg The argument to pass to sync_func
 */
void cpu_reset(CPU* cpu, Memory* mem, void (*sync_func)(void*), void* sync_arg);

/**
 * @brief Advances the CPU by 1 instruction.
 *
 * @param cpu The CPU to step forward
 * @param mem The memory bus the CPU is connected to
 * @param sync_func The function to call every time the system needs to be advanced by 1 CPU clock
 * tick
 * @param sync_arg The argument to pass to sync_func
 */
void cpu_step(CPU* cpu, Memory* mem, void (*sync_func)(void*), void* sync_arg);

#endif
