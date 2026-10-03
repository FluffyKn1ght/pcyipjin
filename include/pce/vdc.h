/**
 * @file pce/vdc.h
 * @brief Emulates a HuC6270 VDC (Video Display Controller) chip
 */

#ifndef _PCYIPJIN_PCE_VDC_H
#define _PCYIPJIN_PCE_VDC_H

#include "memory.h"

/**
 * @brief Represents the different registers of a HuC6270 VDC chip
 */
typedef enum : u8 {
    VDC_REG_MAWR = 0, /**< Memory Address Write */
    VDC_REG_MARR,     /**< Memory Address Read */
    VDC_REG_VWR_VRR,  /**< VRAM Data Read/Write */
    VDC_REG_UNUSED03, /**< Unused */
    VDC_REG_UNUSED04, /**< Unused, BUT "If number "04" is set in the AR, the system can not be
                         assured of normal operation." */
    VDC_REG_CR,       /**< Control */
    VDC_REG_RCR,      /**< Scanning Line Detection */
    VDC_REG_BXR,      /**< BGX Scroll */
    VDC_REG_BYR,      /**< BGY Scroll */
    VDC_REG_MWR,      /**< Memory Access Width */
    VDC_REG_HSR,      /**< Horizontal Sync */
    VDC_REG_HDR,      /**< Horizontal Display */
    VDC_REG_VPR,      /**< Vertical Sync */
    VDC_REG_VDW,      /**< Vertical Display */
    VDC_REG_VCR,      /** Vertical Display End Position */
    VDC_REG_DCR,      /**< Block Transfer Control */
    VDC_REG_SOUR,     /** Block Transfer Source Address */
    VDC_REG_DESR,     /** Block Transfer Destination Address */
    VDC_REG_LENR,     /** Block Transfer Length */
    VDC_REG_DVSSR,    /** VRAM<=>SATB Block Transfer Source */
} VDCRegister;

/**
 * @brief Represents the status register of a HuC6270 VDC chip
 */
typedef union {
    u8 byte; /**< The entire lower byte of the status value (upper byte is always 0) */
    struct {
        bool collide : 1;           /**< Sprite 0 collided with sprites #1-#63 */
        bool over : 1;              /**< Sprite overflow/render error */
        bool scnl_cmp : 1;          /**< Scanline compare */
        bool blt_vram_satb_end : 1; /**< VRAM<=>SATB block transfer ended */
        bool blt_vram_end : 1;      /**< VRAM<=>VRAM block transfer ended */
        bool vblank : 1;            /**< Vertical blanking */
        bool busy : 1;              /**< VRAM busy */
    } bits; /**< The individual flags in the lower byte of the status value */
} VDCStatus;

/**
 * @brief Represents the state of a HuC6270 VDC chip
 */
typedef struct {
    VDCRegister reg;  /**< The currently selected register */
    VDCStatus status; /**< The status register of the VDC */
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
