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
    VDC_REG_VDR,      /**< Vertical Display */
    VDC_REG_VCR,      /** Vertical Display End Position */
    VDC_REG_DCR,      /**< Block Transfer Control */
    VDC_REG_SOUR,     /** Block Transfer Source Address */
    VDC_REG_DESR,     /** Block Transfer Destination Address */
    VDC_REG_LENR,     /** Block Transfer Length */
    VDC_REG_DVSSR,    /** VRAM<=>SATB Block Transfer Source */
} VDCRegister;

/**
 * @brief Represents the different states of a HuC6270 VDC
 */
typedef enum : u8 {
    VDC_STATE_VBLANK_AFTER = 0, /** [VDS] Blank scanlines after VSync pulse (VBlank)
                                 */
    VDC_STATE_HSYNC,            /** [HSW] Horizontal sync pulse (HBlank) */
    VDC_STATE_HBLANK_BEFORE,    /** [HDS] Blank space before scanline output (HBlank) */
    VDC_STATE_RENDER,           /** [HDW] Actively rendering scanline */
    VDC_STATE_HBLANK_AFTER,     /** [HDE] Blank space after scaline output (HBlank) */
    VDC_STATE_VBLANK_BEFORE,    /** [VCR] Blank scanlines after picture output (VBlank/BURST) */
    VDC_STATE_VSYNC,            /** [VSW] Vertical sync pulse (VBlank/BURST) */
} VDCState;
#define VDC_STATE_IS_HBLANK(n)                                                                     \
    ((n) == VDC_STATE_HSYNC || (n) == VDC_STATE_HBLANK_BEFORE || (n) == VDC_STATE_HBLANK_AFTER)
#define VDC_STATE_IS_VBLANK(n)                                                                     \
    ((n) == VDC_STATE_VBLANK_BEFORE || (n) == VDC_STATE_VSYNC || (n) == VDC_STATE_VSYNC_AFTER)

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
typedef u16 VDC_VRAM[(64 * 1024) / 2];

/**
 * @brief Represents a sprite attribute table from the SATB inside of a HuC6270
 */
typedef union {
    u16 word[4]; /**< Raw access to the SAT data, as 4 consecutive 16-bit words */

    struct {
        u16 y : 10;       /**< Y coordinate (top of screen is 64) */
        u16 x : 10;       /**< X coordinate (left of screen is 32) */
        u16 pattern : 11; /**< Pattern code (10 upper bits of VRAM address + lowest bit switches
                             between SG0-SG1 and SG2-SG3) */
        struct {
            u8 pal : 4; /**< Area color code */
            u8 _pad1 : 3;
            bool spbg : 1; /**< Sprite/background priority */
            u8 cgx : 1;    /**< Consecutive X rendering */
            u8 _pad2 : 2;
            bool xflip : 1; /**< Horizontal flip */
            u8 cgy : 2;     /**< Consecutive Y rendering */
            u8 _pad3 : 1;
            bool yflip : 1; /**< Vertical flip */
        };
    } sat; /** The actual SAT data fields */
} VDCSprite;

/**
 * @brief Represents the 512 bytes of SATB (Sprite Attribute Table Buffer) RAM inside of a HuC6270
 */
typedef VDCSprite VDC_SATB[64];

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

        MAIncrementAmount iw : 2; /**< Memory Address Read/Write Register Increment Select */
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
        VDCScreenSize screen : 3; /**< Virtual screen size (applies next frame) */
        bool cg_mode : 1; /**< Use character generator bitplanes 2 and 3 vs 0 and 1 (applies next
                             scanline) */
    };
} VDCMemoryWidth;

/**
 * @brief Represents the value of the DCR register on a HuC6270
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
 * @brief Represents the value of the HSR register on a HuC6270
 */
typedef union {
    u16 word;

    struct {
        u8 hsw : 5; /**< Horizontal Sync Pulse Width (+1) */
        u8 hds : 7; /**< Horizontal Display Start Position (+1) */
    };
} VDC_HSRRegister;

/**
 * @brief Represents the value of the HDR register on a HuC6270
 */
typedef union {
    u16 word;

    struct {
        u8 hdw : 7; /**< Horizontal Display Width (+1) */
        u8 hde : 7; /**< Horizontal Display End Position (+1) */
    };
} VDC_HDRRegister;

/**
 * @brief Represents the value of the VPR register on a HuC6270
 */
typedef union {
    u16 word;

    struct {
        u8 vsw : 5; /**< Vertical Sync Pulse Width (+1) */
        u8 vds;     /**< Vertical Display Start Position (+2) */
    };
} VDC_VPRRegister;

/**
 * @brief Represents the state of a HuC6270 VDC chip
 */
typedef struct {
    VDCState state;   /**< The current state of the VDC */
    u16 state_cycles; /**< Cycles left before state switch */

    VDCRegister reg;  /**< The currently selected register */
    VDCStatus status; /**< The status register of the VDC */

    VDC_VRAM* vram; /**< The 64KiB of VRAM connected to the VDC */
    VDC_SATB* satb; /**< The 512 bytes (256 words) of SATB RAM located inside the VDC */

    u16 mawr;            /**< The current value of the MAWR register */
    u16 marr;            /**< The current value of the MAWR register */
    VDCControl cr;       /**< The current value of the CR register */
    u16 rcr;             /**< The current value of the RCR register */
    u16 bxr;             /**< The current value of the BXR register (applies next scanline) */
    u16 byr;             /**< The current value of the BYR register (applies next scanline) */
    VDCMemoryWidth mwr;  /**< The current value of the MWR register */
    VDC_DCRRegister dcr; /**< The current value of the DCR register */

    u16 sour; /**< The current value of the SOUR register */
    u16 desr; /**< The current value of the DESR register */
    u16 lenr; /**< The current value of the LENR register */

    u16 dvssr; /**< The current value of the DVSSR register */

    VDC_DMAState vram_dma_state; /**< VRAM=>VRAM block-transfer state */
    VDC_DMAState satb_dma_state; /**< VRAM=>SATB block-transfer state */

    VDC_HSRRegister hsr; /**< The current value of the HSR register */
    VDC_HDRRegister hdr; /**< The current value of the HDR register */
    VDC_VPRRegister vpr; /**< The current value of the VPR register */
    u16 vdw : 9;         /**< Current VDW setting */
    u8 vcr;              /**< Current VCR setting */

    bool irq; /**< If true, an IRQ1 interrupt was requested by the VDC (for one reason or
                 another) */

    bool bg_visible;      /**< Whether to show the background */
    bool sprites_visible; /**< Whether to show the sprites */

    u16 _bgscroll_x;    /**< (internal) The actual background X scroll value used when rendering */
    u16 _bgscroll_y;    /**< (internal) The actual background Y scroll value used when rendering */
    u16 _screen_size_x; /**< (internal) The actual X size of the virtual screen used when rendering
                          (in BG tiles) */
    u16 _screen_size_y; /**< (internal) The actual Y size of the virtual screen used when rendering
                          (in BG tiles) */
    bool _render_sprites; /**< (internal) Whether sprites are to rendererd this scanline */
    bool _render_bg;      /**< (internal) Whether the BG is to be rendererd this scanline */
    bool _use_alt_cg; /**< (internal) Whether the CG will use blocks 0 and 1 or 2 or 3 in 4 cycle
                         mode */
} VDC;

/**
 * @brief Creates a new VDC instance
 *
 * @return The created VDC
 */
VDC* vdc_create();

/**
 * @brief Destroys and frees a VDC instance
 *
 * @param vdc The VDC instance to free
 */
void vdc_destroy(VDC* vdc);

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
 * @note This should be called by the VCE so the VDC is synced up to its pixel clock
 *
 * @param vdcptr The VDC to step forward
 *
 * @return The color the VCE should output to the CRT
 */
u8 vdc_step(void* vdcptr);

#endif
