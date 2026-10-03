/**
 * @file pce/vdc.h
 * @brief Emulates a HuC6270 VDC (Video Display Controller) chip
 */

#ifndef _PCYIPJIN_PCE_VDC_H
#define _PCYIPJIN_PCE_VDC_H

/**
 * @brief Represents the state of a HuC6270 VDC chip
 */
#include "memory.h"
typedef struct {
    // TODO: implement
} VDC;

/**
 * @brief Resets the VDC
 *
 * @param vdc The VDC to reset
 */
void vdc_reset(VDC* vdc);

/**
 * @brief Reads data from the VDC
 *
 * @warning Invalid address will cause an assert fail!
 *
 * @see memory.h::BusReadFunc
 */
u8 vdc_read(void* vdcptr, MemoryAccess access, u16 addr);

/**
 * @brief Writes data to the VDC
 *
 * @warning Invalid address will cause an assert fail!
 *
 * @see memory.h::BusWriteFunc
 */
void vdc_write(void* vdcptr, MemoryAccess access, u16 addr, u8 value);

/**
 * @brief Steps the provided VDC by 1 pixel clock cycle forward
 *
 * @note This should be called by the VCE to sync the VDC up to its pixel clock
 *
 * @param vdcptr The VDC to step forward
 */
void vdc_step(void* vdcptr);

#endif
