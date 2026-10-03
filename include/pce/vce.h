/**
 * @file pce/vce.h
 * @brief Emulates a HuC6260 VCE (Video Color Encoder) chip
 */

#ifndef _PCYIPJIN_PCE_VCE_H
#define _PCYIPJIN_PCE_VCE_H

#include "memory.h"

#define VCE_CLOCKDIV_10MHZ 2
#define VCE_CLOCKDIV_7MHZ 3
#define VCE_CLOCKDIV_5MHZ 4

/**
 * @brief Represents an BRG (blue-red-green) color value, with 3 bits of depth per channel (9 bits
 * total)
 */
typedef union {
    struct __attribute__((packed)) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        u8 b : 3; /**< Blue channel */
        u8 r : 3; /**< Red channel */
        u8 g : 3; /**< Green channel */
#else
#error Unsupported byte order
#endif
    };
    u16 value; /**< Complete 9-bit value */
} BRG333;

/**
 * @brief Represents the state of a HuC6260 VCE chip
 */
typedef struct {
    union {
        BRG333 color[512]; /**< Video memory (VRAM) of the VCE chip (512 9-bit bytes) */
        u8 byte[1024];
    } vram;

    u16 cta; /**< Color Table Address register */

    u16 clock_modulo;  /**< VCE clock modulo */
    s16 clock_counter; /**< VCE clock counter */
} VCE;

/**
 * @brief Resets the provided VCE
 *
 * @warning Invalid address will cause an assert fail!
 *
 * @param vce The VCE to reset
 */
void vce_reset(VCE* vce);

/**
 * @brief Reads data from the VCE
 *
 * @warning Invalid address will cause an assert fail!
 *
 * @see memory.h::BusReadFunc
 */
u8 vce_read(void* vceptr, MemoryAccess access, u32 addr);

/**
 * @brief Writes data to the VCE
 *
 * @warning Invalid address will cause an assert fail!
 *
 * @see memory.h::BusWriteFunc
 */
void vce_write(void* vceptr, MemoryAccess access, u32 addr, u8 value);

/**
 * @brief Ticks the internal clock of the provided VCE
 *
 * @param vce The VCE to tick
 * @param clocks The amount of master clocks ticks that happened
 * @param vdc_tick_func The function to call to tick the VDC every VCE clock cycle
 * @param vdc_tick_arg The argument to pass to vdc_tick_func
 */
void vce_tick(VCE* vce, u16 clocks, void (*vdc_tick_func)(void*), void* vdc_tick_arg);

#endif
