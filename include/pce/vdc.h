/**
 * @file pce/vdc.h
 * @brief Emulates a HuC6270 VDC (Video Display Controller) chip
 */

#ifndef _PCYIPJIN_PCE_VDC_H
#define _PCYIPJIN_PCE_VDC_H

/**
 * @brief Represents the state of a HuC6270 VDC chip
 */
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
 * @brief Writes data to the VDC (i.e. when running a ST0/ST1/ST2 instruction)
 *
 * @warning Invalid address will cause an assert fail!
 *
 * @param vdc The VDC to write the data to
 * @param addr Address to write to (0, 1 or 2)
 * @param value The value to write
 */
void vdc_write(VDC* vdc, u8 addr, u8 value);

#endif
