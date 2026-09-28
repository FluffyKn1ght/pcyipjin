/**
 * @file pce/cpu.h
 * @brief Emulates a HuC6280 (MOS6502 variant) CPU
 */

#ifndef _PCYIPJIN_PCE_CPU_H
#define _PCYIPJIN_PCE_CPU_H

#include "pce/memory.h"
#include "stdbool.h"
#include "utils/yield.h"

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
} CPU;

/**
 * @brief Represents different checks a CPUTest can perform
 */
typedef struct {
    bool acc : 1;         /**< Whether to check ACC */
    bool x : 1;           /**< Whether to check X */
    bool y : 1;           /**< Whether to check Y */
    bool sp : 1;          /**< Whether to check SP */
    bool pc : 1;          /**< Whether to check PC */
    bool cycle_count : 1; /**< Whether to check cycle count */
    bool flags : 1;       /**< Whether to check flags/status */
} CPUTestChecks;

typedef struct {
    u8 acc; /**< Expected ACC register */
    u8 x;   /**< Expected X register */
    u8 y;   /**< Expected Y register */
    u8 sp;  /**< Expected stack pointer */

    u16 pc;          /**< Expected program counter */
    u16 cycle_count; /**< Expected CPU cycle count */

    struct {
        union {
            CPUStatus status; /**< Expected status register, as a CPUStatus bitfield */
            u8 p;             /**< Expected status register, as a byte */
        };

        CPUTestChecks check;
    };
} CPUTest;

/**
 * @brief Resets the CPU state.
 *
 * @param cpu The CPU to reset
 */
void cpu_reset(CPU* cpu);

typedef struct {
    CPU* cpu;    /**< The CPU to advance */
    Memory* mem; /**< The memory bus the CPU is connected to */
} cpu_step_ctx;

/**
 * @brief Advances the CPU by 1 CPU clock cycle.
 *
 * @note This is a coroutine, call with `coro_call(ybuf, cpu_step, &(cpu_step_ctx){cpu, mem}))`
 *
 * @details This doesn't correspond to executing 1 instruction. Instead, every time this function
 * is executed, the CPU performs one bus access (read/write), advancing its internal state
 * accordingly. This means that 1 instruction can take multiple cpu_step() calls to execute fully.
 * This is also known as being "cycle accurate".
 *
 * @param ctxptr (cpu_step_ctx*) A struct containing the params to the function
 */
coroutine cpu_step(void* ctxptr);

/**
 * @brief Runs the provided program on the CPU and checks if the final CPU state matches the
 * expected state
 *
 * @note The CPU state passed into this function WILL BE FREED. The program passed into the function
 * (and the CPUTest struct) will NOT be freed.
 *
 * @details This will also print information regarding the test result/details and comparisons if it
 * fails. If the provided test doesn't have any checks enabled, it will fail automatically.
 *
 * @param cpu The initial CPU state (NULL to allocate a new one)
 * @param program Pointer to program data (will be put into RAM at address 0x00)
 * @param program_size The size of the program
 * @param test Expected result of program
 * @param log Whether to enable more detailed logging of cycle stuffs
 *
 * @return Whether the test passed or not
 */
bool cpu_test(CPU* cpu, const u8* program, u32 program_size, CPUTest* test, bool log_cycles);

#endif
