/**
 * @file pce/vdc.h
 * @brief Emulates a HuC6270 Video Display Controller (VDC)
 */

#ifndef _PCYIPJIN_PCE_VDC_H
#define _PCYIPJIN_PCE_VDC_H

#include "utils/queue.h"
#define VDC_PREFETCH_TILE_COUNT 2

#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#error Unsupported byte order
#endif

/**
 * @brief Represents different registers of the HuC6270 VDC
 *
 * @note Assuming A1 == 1
 */
typedef enum : u8 {
    VDC_REG_MAWR,
    VDC_REG_MARR,
    VDC_REG_VWRW,
    VDC_REG_UNK03,
    VDC_REG_UNK04,
    VDC_REG_CR,
    VDC_REG_RCR,
    VDC_REG_BXR,
    VDC_REG_BYR,
    VDC_REG_MWR,
    VDC_REG_HSR,
    VDC_REG_HDR,
    VDC_REG_VPR,
    VDC_REG_VDW,
    VDC_REG_VCR,
    VDC_REG_DCR,
    VDC_REG_SOUR,
    VDC_REG_DESR,
    VDC_REG_LENR,
    VDC_REG_DVSSR,
    VDC_REG_COUNT
} PCE_VDCRegister;

/**
 * @brief Represents different values of the IW register on the HuC6270 VDC
 */
typedef enum : u8 {
    VDC_IW_0X1 = 0x1,
    VDC_IW_0X20 = 0x20,
    VDC_IW_0X40 = 0x40,
    VDC_IW_0X80 = 0x80
} PCE_VDCAddrIncrement;

/**
 * @brief Represents different VRAM (CG) access widths on the HuC6270 VDC
 */
typedef enum : u8 {
    VDC_VM_1,  /**< 8 accesses, CPU BAT CPU --- CPU CG0 CPU CG1 */
    VDC_VM_2A, /**< 4 accesses, BAT CPU CG0 CG1 */
    VDC_VM_2B, /**< 4 accesses, BAT CPU CG0 CG1 */
    VDC_VM_4,  /**< 2 accesses, only 1 bitplane, BAT CG0/CG1 */
} PCE_VDC_VRAMAccessWidth;

/**
 * @brief Represents different SG access widths on the HuC6270 VDC
 */
typedef enum : u8 {
    VDC_SM_1,     /**< 8 accesses, SP0 SP1 SP2 SP3 SP0 SP1 SP2 SP3 */
    VDC_SM_2_LSB, /**< LSB of pattern code selects (SP0, SP1) or (SP2, SP3), 4 accesses, SP0/SP2
                     SP1/SP3 SP0/SP2 SP1/SP3 */
    VDC_SM_2,     /**< SP0 SP1 SP0 SP1 */
    VDC_SM_4,     /**< 2 consecutive character cycles, SP0 SP1 | SP2 SP3 */
} PCE_VDCSpriteAccessWidth;

/**
 * @brief Represents different virtual screen sizes on the HuC6270 VDC
 */
typedef enum : u8 {
    VDC_SCREEN_32_32,
    VDC_SCREEN_64_32,
    VDC_SCREEN_128_32,
    VDC_SCREEN_128_32_,
    VDC_SCREEN_32_64,
    VDC_SCREEN_64_64,
    VDC_SCREEN_128_64,
    VDC_SCREEN_128_64_,
    VDC_SCREEN_COUNT
} PCE_VDC_VScreenSize;

/**
 * @brief Represents different states of SATB DMA on the HuC6270 VDC
 */
typedef enum : u8 {
    VDC_SATBDMA_IDLE,   /**< SATB DMA wasn't armed yet */
    VDC_SATBDMA_ARMED,  /**< SATB DMA was armed but it's not VBlank yet */
    VDC_SATBDMA_ACTIVE, /**< SATB DMA is currently in progress */
} PCE_VDC_SATBDMAState;

/**
 * @brief Represents different External Sync settings on the HuC6270 VDC
 */
typedef enum : u8 {
    VDC_EXSYNC_FULL,    /**< Both VSYNC and HSYNC work as input (slave mode?) */
    VDC_EXSYNC_VSYNC,   /**< VSYNC works as input, HSYNC is still an output */
    VDC_EXSYNC_INVALID, /**< Invalid (logically this would make VSYNC output and HSYNC input) */
    VDC_EXSYNC_OFF,     /**< Both VSYNC and HSYNC work as output (master mode?) */
} PCE_VDCExternalSync;

/**
 * @brief Represents different states of the HuC6270 VDC
 */
typedef enum : s8 {
    VDC_STATE_BURST =
        -1, /**< FBlank, VDC isn't doing anything for the entire frame and VRAM is fully open */
    VDC_STATE_VBLANK_VDS = 0,   /**< VBlank before frame start */
    VDC_STATE_HSYNC,            /**< HSync pulse */
    VDC_STATE_HBLANK_HDS,       /**< HBlank before scanline start */
    VDC_STATE_HBLANK_HDS_PRIME, /**< The last 2 cycles of HBlank when the VDC prepares data for BG
                                   rendering */
    VDC_STATE_RENDER,           /**< Actual rendering */
    VDC_STATE_HBLANK_HDE,       /**< HBlank after scanline end */
    VDC_STATE_VBLANK_VCR,       /**< VBlank after frame end */
    VDC_STATE_VSYNC,            /**< VSync pulse */
    VDC_STATE_COUNT
} PCE_VDCState;

/**
 * @brief Represents different sprite CGY settings on the HuC6270 VDC
 */
typedef enum : u8 { VDC_CGY_OFF, VDC_CGY_2CGY, VDC_CGY_INVALID, VDC_CGY_4CGY } VDCSpriteCGY;

/**
 * @brief Represents a Sprite Attribute Table (SAT) entry on the HuC6270 VDC
 */
typedef struct {
    u16 y : 10;  /**< Sprite Y coordinate */
    u16 x : 10;  /**< Sprite X coordinate */
    u16 pc : 11; /**< Pattern code */

    u8 color : 4; /**< Sprite color */
    u8 _pad1 : 3;
    bool spbg : 1; /**< Sprite/background priority */
    bool cgx : 1;  /**< Consecutive X display */
    u8 _pad2 : 2;
    bool xflip : 1;       /**< Mirror sprite(s) on X axis */
    VDCSpriteCGY cgy : 2; /**< Consecutive Y display */
    u8 _pad3 : 1;
    bool yflip : 1; /**< Mirror sprite(s) on Y axis */
} PCE_VDCSprite;

/**
 * @brief Represents a Background Attribute Table (BAT) entry on the HuC6270 VDC
 */
typedef union {
    u16 word; /**< The entire entry as a single word */
    struct {
        u16 charcode
            : 12; /**< Character code, upper 12 bits of a 16-bit pointer to tile pattern data */
        u8 cgcolor : 4; /**< Upper 4 bits of the VCE CRAM address, basically a tile pallete */
    };
} PCE_VDC_BGAttrTableEntry;

/**
 * @brief Represents the output of a HuC6270 VDC chip
 */
typedef struct {
    u8 bg;  /**< Background color */
    u8 spr; /**< Sprite color */

    bool spbg : 1;  /**< Whether the sprite color has priority over the background color */
    bool vsync : 1; /**< Current VSYNC pin output value */
    bool hsync : 1; /**< Current HSYNC pin output value */
} PCE_VDCOutput;

/**
 * @brief Represents a HuC6270 Video Display Controller (VDC) chip
 */
typedef struct {
    PCE_VDCSprite* satb; /**< Sprite Attribute Table Buffer (SATB). This should point to a
                           VDCSprite[64]. */
    u16* vram;           /**< Video RAM (VRAM). This should point to a 32K word (64KB) buffer */

    PCE_VDCRegister ar : 5; /** Address Register */

    union {
        u16 word : 7;
        struct {
            bool cr : 1;  /**< Collision Detect */
            bool or : 1;  /**< Over Detect */
            bool rr : 1;  /**< Scanning Line Detect */
            bool ds : 1;  /**< SATB DMA end */
            bool dv : 1;  /**< VRAM DMA graceful end */
            bool vd : 1;  /**< Vertical Blanking Period Detect (VBlank) */
            bool bsy : 1; /**< VRAM latch buffer full */
        };
    } sr; /**< Status Register */

    u16 mawr : 15; /**< Memory Address Write */
    u16 marr : 15; /**< Memory Address Read */

    u16 vram_read_buf;         /**< VRAM read access buffer */
    u16 vram_write_buf;        /**< VRAM write access buffer */
    bool vram_access_is_write; /**< true = VRAM write, false = VRAM read */

    union {
        u16 word : 13;
        struct {
            struct {
                bool cc : 1;             /**< Collision Detect */
                bool oc : 1;             /**< Over Detect */
                bool rc : 1;             /**< Scanning Line Detect */
                bool vc : 1;             /**< Vertical Blanking Period Detect */
            } ie;                        /**< Interrupt Request Enable */
            PCE_VDCExternalSync ex : 2;  /**< External Sync (does nothing, see VDC::slave) */
            bool sb : 1;                 /**< Sprite Blanking [next line] */
            bool bb : 1;                 /**< Background Blanking [next line] */
            u8 te : 1;                   /**< DISP Output Select (not used) */
            bool dr : 1;                 /**< Dynamic RAM Refresh (not used) */
            PCE_VDCAddrIncrement iw : 2; /**< Memory Address Read/Write Register Increment Select */
        };
    } cr; /**< Control Register */

    u16 rcr : 10; /**< Scanning Line Detection Register */

    u16 bxr : 10; /**< BGX Scroll [next line] */
    u16 byr : 9;  /**< BGY Scroll [next line] */

    union {
        u16 word : 8;
        struct {
            PCE_VDC_VRAMAccessWidth vm : 2;  /**< VRAM (CG) Access Width [next frame] */
            PCE_VDCSpriteAccessWidth sm : 2; /**< SG Access Width [next frame] */
            PCE_VDC_VScreenSize screen : 3;  /**< Virtual Screen Size [next frame] */
            bool cm : 1; /**< CG block select for VM 4-clock mode (true = ch2, ch3; false = ch0,
                            ch1) [next line] */
        };
    } mwr; /**< Memory Access Width Register */

    union {
        u16 word : 15;
        struct {
            u8 hsw : 5; /**< Horizontal Sync Pulse Width - 1 [next frame] */
            u8 hds : 7; /**< Horizontal Display Start Position - 1 [next frame] */
        };
    } hsr; /**< Horizontal Sync Register */

    union {
        u16 word : 15;
        struct {
            u8 hdw : 7; /**< Horizontal Display Width - 1 [next frame] */
            u8 hde : 7; /**< Horizontal Display End Position - 1 [next frame] */
        };
    } hdr; /**< Horizontal Display Register */

    union {
        u16 word : 16;
        struct {
            u8 vsw : 5; /**< Vertical Sync Pulse Width - 1 [next frame] */
            u8 vds;     /**< Vertical Display Start Position - 2 [next frame] */
        };
    } vpr; /**< Vertical Sync Register */

    union {
        u16 word : 9;
        struct {
            u16 vdw : 9; /**< Vertical Display Width - 1 [next frame] */
        };
    } vdw; /**< Vertical Display Width Register */

    union {
        u16 word : 8;
        struct {
            u8 vcr; /**< Vertical Display End Position [next frame] */
        };
    } vcr; /**< Vertical Display End Position Register */

    union {
        u16 word : 5;
        struct {
            bool dsc : 1; /**< SATB DMA Completed IRQ Enable */
            bool dvc : 1; /**< VRAM DMA Completed IRQ Enable */
            bool sid : 1; /**< VRAM DMA Source Address Increment/Decrement */
            bool did : 1; /**< VRAM DMA Destination Address Increment/Decrement */
            bool dsr : 1; /**< SATB DMA Automatic Start */
        };
    } dcr; /**< Block Transfer Control Register */

    u16 sour : 15; /**< VRAM DMA Source */
    u16 desr : 15; /**< VRAM DMA Destination */
    u16 lenr;      /**< VRAM DMA Length - 1 */

    u16 dvssr : 15; /**< SATB DMA Source */

    bool vram_dma;                 /**< Whether VRAM DMA is currently underway */
    PCE_VDC_SATBDMAState satb_dma; /**< The current state of SATB DMA */

    bool irq1; /**< Interrupt Request output pin */

    PCE_VDCState state;  /**< Current VDC state */
    s16 charcycle_count; /**< Character cycle counter */
    s16 dotcycle_count;  /**< Dotcycle counter */
    s8 _vram_access_dotcycles;

    u16 scanline : 9; /**< Current scanline */
    u16 pixel : 11;   /**< Current pixel */

    PCE_VDC_BGAttrTableEntry _temp_bat;
    u16 _temp_cg[2];

    Queue* _pixel_fifo;

    // Relatched every VBlank

    PCE_VDC_VRAMAccessWidth _latched_vm;
    PCE_VDCSpriteAccessWidth _latched_sm;
    PCE_VDC_VScreenSize _latched_screen;
    u8 _latched_vscr_width : 7, _latched_vscr_height : 5;
    u8 _latched_hsw : 5, _latched_hds : 7, _latched_hdw : 7, _latched_hde : 7;
    u8 _latched_vsw : 5, _latched_vds, _latched_vcr;
    u16 _latched_vdw : 9;

    // Relatched every HBlank

    bool _latched_cm;
    u16 _latched_bgx : 10, _latched_bgy : 9;
    bool _latched_bb, _latched_sb;
} HuC6270;

/**
 * @brief Creates a new VDC instance
 *
 * @return The created VDC
 */
HuC6270* pce_vdc_create();

/**
 * @brief Destroys a VDC instance
 *
 * @param vdc The VDC to destroy
 */
void pce_vdc_destroy(HuC6270* vdc);

/**
 * @brief Resets the provided VDC
 *
 * @param vdc The VDC to reset
 */
void pce_vdc_reset(HuC6270* vdc);

/**
 * @brief Steps the VDC forward by one pixel clock, returning its output.
 *
 * @param vdc The VDC to step forward
 *
 * @return The 9-bit output values of the VDC, for use with vce_render()
 *
 * @see pce/vce.h::vce_encode()
 */
PCE_VDCOutput pce_vdc_step(HuC6270* vdc);

/**
 * @brief Reads a byte from the VDC
 *
 * @param vdc The VDC to read from
 * @param addr The address being read from, relative to the VCE's register I/O space
 *
 * @return The value that was read
 */
u8 pce_vdc_read(HuC6270* vdc, u8 addr);

/**
 * @brief Writes a byte to the VDC
 *
 * @param vdc The VDC to read from
 * @param addr The address being written to, relative to the VDC's register I/O space
 * @param value The value to write
 */
void pce_vdc_write(HuC6270* vdc, u8 addr, u8 value);

extern const u16 PCE_VDC_VSCREEN_SIZES[VDC_SCREEN_COUNT][2];

#endif
