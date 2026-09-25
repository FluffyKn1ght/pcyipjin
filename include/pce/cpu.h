/**
 * @file pce/cpu.h
 * @brief Emulates a HuC6280 (MOS6502 variant) CPU
 */

#ifndef _PCEEMU_PCE_CPU_H
#define _PCEEMU_PCE_CPU_H

#include "pce/memory.h"
#include "stdbool.h"

#define CPU_TMA_MPR0 0x1
#define CPU_TMA_MPR1 0x2
#define CPU_TMA_MPR2 0x4
#define CPU_TMA_MPR3 0x8
#define CPU_TMA_MPR4 0x10
#define CPU_TMA_MPR5 0x20
#define CPU_TMA_MPR6 0x40
#define CPU_TMA_MPR7 0x80

#define CPU_VEC_RESET 0x1FFE
#define CPU_VEC_NMI 0xFFFC
#define CPU_VEC_TIMER 0xFFFA
#define CPU_VEC_IRQ1 0xFFF8
#define CPU_VEC_IRQ2 0xFFF6 // also BRK

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
    u16 pc; /**< Program counter */
    u8 sp;  /**< Stack pointer */

    union {
        CPUStatus status; /**< Status register, as a CPUStatus bitfield */
        u8 p;             /**< Status register, as a byte */
    };

    // TODO: SH, DH, LH registers
    // TODO: Interrupt stuffs
    // TODO: Timer
    // TODO: I/O (K and O ports)

    u8 mpr[8]; /**< Mapping registers (MPR0-MPR7) */
} CPU;

/**
 * @brief Resets the CPU state.
 *
 * @param cpu The CPU to reset
 */
void cpu_reset(CPU* cpu);

/**
 * @brief Advances the CPU by 1 CPU clock cycle.
 *
 * @details This doesn't correspond to 1 opcode. Instead, every time this function
 * is executed, the CPU performs one bus access (read/write), advancing its internal state
 * accordingly. This means that 1 instruction can take multiple cpu_step() calls to execute fully.
 * This is also known as being "cycle accurate".
 *
 * @param cpu The CPU whose state to advance.
 * @param mem The memory bus the CPU should access.
 */
void cpu_step(CPU* cpu, Memory* mem);

#endif
