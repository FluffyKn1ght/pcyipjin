/**
 * @file pce/cpu.h
 * @brief Emulates a HuC6280 (MOS6502 variant) CPU
 */

#ifndef _PCYIPJIN_PCE_CPU_H
#define _PCYIPJIN_PCE_CPU_H

#include "callbacks.h"
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
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    union {
        struct {
            u8 acc; /**< Accumulator register (ACC) */
            u8 lh;  /**< Block transfer length high (LH). Paired with ACC to get a 16-bit value */

            u8 x;  /**< X register */
            u8 sh; /**< Block transfer source high (SH). Paired with X to get a 16-bit value */

            u8 y;  /**< Y register */
            u8 dh; /**< Block transfer destination high (DH). Paired with Y to get a 16-bit value */
        };

        u16 blt_length; /**< Block transfer length register pair (LH + ACC) */
        u16 blt_source; /**< Block transfer source register pair (SH + X) */
        u16 blt_dest;   /**< Block transfer destination register pair (DH + Y) */
    };
#else
#error Unsupported byte order
#endif

    u8 sp;  /**< Stack pointer */
    u16 pc; /**< Program counter */

    union {
        CPUStatus status; /**< Status register, as a CPUStatus bitfield */
        u8 p;             /**< Status register, as a byte */
    };

    // TODO: Interrupt stuffs
    // TODO: Timer
    // TODO: I/O (K and O ports)

    u8 mpr[8]; /**< Mapping registers (MPR0-MPR7) */

    bool high_speed; /**< Whether the CPU is running at 7MHz high speed */

    bool _alu_discard;      /**< (internal) Whether the next ALU result should be discarded (i.e.
                               cmp/cpx/cpy) */
    bool _blt_alternate[2]; /**< (internal) Whether the alternating source/destination address will
                            be incremented or decremented */
    u8 _blt_alternate_idx;  /**< (internal) Which boolean to use in _blt_alternate */
} CPU;

/**
 * @brief Resets the CPU state.
 *
 * @param cpu The CPU to reset
 * @param mem The memory bus the CPU is connected to
 * @param ec Emulator communication callbacks
 */
void cpu_reset(CPU* cpu, Memory* mem, EmuCallbacks* ec);

/**
 * @brief Advances the CPU by 1 instruction.
 *
 * @param cpu The CPU to step forward
 * @param mem The memory bus the CPU is connected to
 * @param ec Emulator communication callbacks
 */
void cpu_step(CPU* cpu, Memory* mem, EmuCallbacks* ctx);

#endif
