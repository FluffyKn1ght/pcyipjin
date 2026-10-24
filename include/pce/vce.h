/**
 * @file pce/vce.h
 * @brief Emulates a HuC6260 Video Color Encoder (VCE)
 */

#define VCE_MAX_X_SCREEN_SIZE 565
#define VCE_VDW_SCANLINE_COUNT 242

#ifndef _PCYIPJIN_PCE_VCE_H
#define _PCYIPJIN_PCE_VCE_H

#include "memory.h"
#include "pce/vdc.h"

#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#error Unsupported byte order
#endif

/**
 * @brief Represents an 8-bit RGB color
 */
typedef union {
    u32 rgb : 24; /**< The entire color as one 24-bit value */
    struct {
        u8 b; /**< im so blue */
        u8 g; /**< im so green */
        u8 r; /**< im so red */
    };
} RGB888;

/**
 * @brief Represents a 9-bit GRB color (3x 3-bit channels) used on the HuC6260 VCE
 */
typedef union {
    u16 word : 9; /**< Combined color data, as a 9-bit word */
    struct {
        u8 b : 3;
        u8 r : 3;
        u8 g : 3;
    }; /**< Individual color channels */
} GRB333;

/**
 * @brief Represents a HuC6260 Video Color Encoder (VCE) chip
 */
typedef struct {
    s16 clock_counter; /**< Master clock counter */
    u16 clock_modulo;  /**< Number of master clocks per VCE clock */

    u16 cta : 9; /**< Color Table Address register */

    u16 current_pos_x : 10; /**< Current X rendering position */
    u8 current_pos_y;       /**< Current Y rendering position */

    bool latched_hsync : 1; /**< Latched HSYNC pin state */
    bool latched_vsync : 1; /**< Latched VSYNC pin state */
    bool busy : 1; /**< If true, the frame buffer is currently being written to and I'll bite you if
                   you try to read from it 3:< */
    bool grayscale : 1; /**< Undocumented (thanks hudson #2 :_3) "strip colorburst" register
                           according to DataCrystal TCRF, this just makes the image grayscale. */

    GRB333* cram; /**< Color RAM. This should point to a GRB333[512] */

    u16 frame_size_x : 10; /**< The max X size of the frame */
    u8 frame_size_y;       /**< The max Y size of the frame */

    RGB888* frame_out; /**< The rendered frame, in 8-bit RGB format. This should point
                          to a buffer that can hold at least `MAX_X_SCREEN_SIZE*MAX_Y_SCREEN_SIZE`
                          RGB888 values. */
} HuC6260;

/**
 * @brief Creates a new VCE instance
 *
 * @return The created VCE
 */
HuC6260* pce_vce_create();

/**
 * @brief Destroys a VCE instance
 *
 * @param vce The VCE to destroy
 */
void pce_vce_destroy(HuC6260* vce);

/**
 * @brief Resets the provided VCE
 *
 * @param vce The VCE to reset
 */
void pce_vce_reset(HuC6260* vce);

/**
 * @brief Ticks the clock of the provided VCE forward.
 *
 * @note This function is also responsible for telling when to step the rest of the graphics
 * pipeline forward, as the VCE is the one that generates the pixel clock.
 *
 * @param vce The VCE whose clock to advance
 *
 * @return Whether it's time to step the graphics pipeline forward
 */
bool pce_vce_tick(HuC6260* vce);

/**
 * @brief Renders one pixel to the screen using the VCE.
 *
 * @param vce The VCE to step forward
 * @param vdc_out Current VDC output
 *
 * @return Whether the frame has finished rendering. (VSync signal changed to true this frame)
 */
bool pce_vce_render(HuC6260* vce, PCE_VDCOutput vdc_out);

/**
 * @brief Reads a byte from the VCE
 *
 * @param vce The VCE to read from
 * @param addr The address being read from, relative to the VCE's register I/O space
 *
 * @return The value that was read
 */
u8 pce_vce_read(HuC6260* vce, u8 addr);

/**
 * @brief Writes a byte to the VCE
 *
 * @param vce The VCE to read from
 * @param addr The address being written to, relative to the VCE's register I/O space
 * @param value The value to write
 */
void pce_vce_write(HuC6260* vce, u8 addr, u8 value);

#endif
