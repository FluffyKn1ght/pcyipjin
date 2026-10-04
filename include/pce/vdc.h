/**
 * @file pce/vdc.h
 * @brief Emulates a HuC6270 VDC (Video Display Controller) chip
 */

#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#error Unsupported byte order
#endif

#ifndef _PCYIPJIN_PCE_VDC_H
#define _PCYIPJIN_PCE_VDC_H

#include "memory.h"

typedef enum : u8 { MAINC_1 = 0, MAINC_20, MAINC_40, MAINC_80 } MAIncrementAmount;

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
 * @brief Represents the different VRAM access width modes on a HuC6270
 */
typedef enum : u8 {
    VDC_VM1_CBCNC0C1 = 0, /**< 1 - CPU BAT CPU --- CPU CG0 CPU CG1 */
    VDC_VM2_BC01,         /**< 2 - BAT CPU CG0 CG1 */
    VDC_VM3_BC01,         /**< same as VM2 */
    VDC_VM4_BAT_CG01      /**< 4 - BAT CG0/CG1 */
} VDC_VRAMAccess;

/**
 * @brief Represents the different sprite access width modes on a HuC6270
 */
typedef enum : u8 {
    VDC_SM1_0123 = 0,  /**< 1 - SP0 SP1 SP2 SP3 SP0 SP1 SP2 SP3 */
    VDC_SM2_SP02_SP13, /**< 2 - SP0/SP2 SP1/SP3 SP0/SP2 SP1/SP3 (LSB of pattern code selects SP0/SP1
                         or SP2/SP3) */
    VDC_SM2_0123,      /**< 2 - SP0 SP1 SP2 SP3 */
    VDC_SM4_02_13 /** SP0SP2 SP1SP3 (SPO-SP3 are fetched during two consecutive character cycles.)
                   */
} VDCSpriteAccess;

/**
 * @brief Represents different virtual screen sizes on a HuC6270
 */
typedef enum : u8 {
    VDC_SCRSIZE_32_32 = 0,
    VDC_SCRSIZE_64_32,
    VDC_SCRSIZE_128_32,
    VDC_SCRSIZE_128_32_,
    VDC_SCRSIZE_32_64,
    VDC_SCRSIZE_64_64,
    VDC_SCRSIZE_128_64,
    VDC_SCRSIZE_128_64_,
} VDCScreenSize;

/**
 * @brief Represents different states of DMA (block transfer) operations
 */
typedef enum : u8 {
    VDC_DMA_IDLE,   /**< DMA transfer is not armed */
    VDC_DMA_ARMED,  /**< DMA trasnfer is armed and will start soon */
    VDC_DMA_ACTIVE, /**< DMA transfer is actively taking place */
} VDC_DMAState;

/**
 * @brief Represents the status register of a HuC6270 VDC chip
 */
typedef union {
    u8 byte; /**< The entire lower byte of the status value (upper byte is always 0) */
    struct {
        bool collide : 1;           /**< (CC) Sprite 0 collided with sprites #1-#63 */
        bool over : 1;              /**< (OC) Sprite overflow/render error */
        bool scnl_cmp : 1;          /**< (RC) Scanline compare */
        bool blt_vram_satb_end : 1; /**< VRAM=>SATB block transfer ended */
        bool blt_vram_end : 1;      /**< VRAM=>VRAM block transfer ended */
        bool vblank : 1;            /**< Vertical blanking */
        bool busy : 1;              /**< VRAM busy */
    } bits; /**< The individual flags in the lower byte of the status value */
} VDCStatus;

/**
 * @brief Represents a single 16-bit of background tile pattern data.
 *
 * @details This equates to a single row of two 1-bit bitmaps.
 */
typedef union {
    struct {
        u8 a;
        u8 b;
    } bitrows; /**< The individual rows of the 2 bitplanes */
    u16 word;  /**< The entire row of tile data */
} VDC_BGTileRow;

/**
 * @brief Represents a single 8x8px background tile, made up of 4 1-bit bitplanes
 */
typedef struct {
    VDC_BGTileRow ch0_ch1[8]; /**< Rows of bitplanes 1 and 2 */
    VDC_BGTileRow ch2_ch3[8]; /**< Rows of bitplanes 3 and 4 */
} VDC_BGTile;

/**
 * @brief Represents a single 16x16px 1-bit bitplane of a sprite tile
 */
typedef u16 VDCSpriteBitplane[16];

/**
 * @brief Represents a single 16x16px sprite tile, made up of 4 1-bit bitplanes
 */
typedef struct {
    VDCSpriteBitplane bitplanes[4]; /**< The 4 16x16px 1-bit bitplanes of the sprite tile */
} VDCSpriteTile;

/**
 * @brief Represents the 64KiB of video memory (VRAM) connected to a HuC6270 VDC
 *
 * @details The VRAM layout itself is entirely software-defined
 */
typedef union {
    u8 byte[64 * 1024];        /**< The entire VRAM as 8-bit blocks */
    u16 word[(64 * 1024) / 2]; /**< The entire VRAM as 16-bit blocks */
} VDC_VRAM;

/**
 * @brief Represents the value of the CR register on a HuC6270
 */
typedef union {
    u16 word;

    struct {
        bool ie_cc : 1; /**< [IE] Collision Detect */
        bool ie_oc : 1; /**< [IE] Over Detect */
        bool ie_rc : 1; /**< [IE] Scanning Line Detect */
        bool ie_vc : 1; /**< [IE] Vertical Blanking Period Detect */

        u8 ex : 2; /**< External Sync */

        bool sb : 1; /**< Sprite Blanking (applies next scanline) */
        bool bb : 1; /**< Background Blanking (applies next scanline) */

        u8 te : 2; /**< DISP Output Select */

        bool dr : 1; /**< Dynamic RAM Refresh */

        MAIncrementAmount iw; /**< Memory Address Read/Write Register Increment Select */
    };
} VDCControl;

/**
 * @brief Represents the value of the MWR register on a HuC6270
 */
typedef union {
    u16 word;

    struct {
        VDC_VRAMAccess vm : 2;    /**< VRAM Access width Mode */
        VDCSpriteAccess sm : 2;   /**< Sprite Access Width Mode */
        VDCScreenSize screen : 3; /**< Virtual screen size */
        bool cg_mode : 1;         /**< Use character generator blocks 2 and 3 vs 0 and 1 */
    };
} VDCMemoryWidth;

/**
 * @brief Represents the value of a DCR register on a HuC6270
 */
typedef union {
    u16 word;

    struct {
        bool dsc : 1; /**< VRAM=>SATB Transfer Complete IRQ Enable */
        bool dvc : 1; /**< VRAM=>VRAM Transfer Complete IRQ Enable */
        bool sid : 1; /**< Source Address INC/DEC */
        bool did : 1; /**< Destination Address INC/DEC */
    };
} VDC_DCRRegister;

/**
 * @brief Represents the state of a HuC6270 VDC chip
 */
typedef struct {
    VDCRegister reg;  /**< The currently selected register */
    VDCStatus status; /**< The status register of the VDC */

    VDC_VRAM* vram; /**< The 64KiB of VRAM connected to the */

    u16 mawr;            /**< The current value of the MAWR register */
    u16 marr;            /**< The current value of the MAWR register */
    VDCControl cr;       /**< The current value of the CR register */
    u16 rcr;             /**< The current value of the RCR register */
    u16 bxr;             /**< The current value of the BXR register */
    u16 byr;             /**< The current value of the BYR register */
    VDCMemoryWidth mwr;  /**< The current value of the MWR register */
    VDC_DCRRegister dcr; /**< The current value of the DCR register */

    u16 sour; /**< The current value of the SOUR register */
    u16 desr; /**< The current value of the DESR register */
    u16 lenr; /**< The current value of the LENR register */

    u16 dvssr; /**< The current value of the DVSSR register */

    VDC_DMAState vram_dma_state; /**< VRAM=>VRAM block-transfer state */
    VDC_DMAState satb_dma_state; /**< VRAM=>SATB block-transfer state */
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
