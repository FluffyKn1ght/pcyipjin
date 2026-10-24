/**
 * @file pce/graphics.h
 * @brief Emulates the graphics system of a PC Engine console (HuC6260 + HuC6270 + HuC6202)
 */

#ifndef _PCYIPJIN_PCE_GRAPHICS_H
#define _PCYIPJIN_PCE_GRAPHICS_H

#include "memory.h"
#include "pce/vce.h"
#include "pce/vdc.h"

#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#error Unsupported byte order
#endif

/**
 * @brief Represents a rendered frame from a PC Engine graphics pipeline
 */
typedef struct {
    u16 width : 10;                                                /**< Frame width */
    u8 height;                                                     /**< Frame height */
    RGB888 pixels[VCE_MAX_X_SCREEN_SIZE * VCE_VDW_SCANLINE_COUNT]; /**< Pixel data */
} PCE_GFXFrame;

/**
 * @brief Represents the state of a graphics pipeline of a PC Engine console (HuC6260 + HuC6270)
 */
typedef struct {
    bool extended : 1; /** Whether to use the SuperGrafx-exclusive extended graphics pipeline (dual
                       VDCs) */
    bool irq1 : 1;     /**< Whether one of the VDCs requested an IRQ1 interrupt */
    bool show_sprites : 1;    /**< Enables/disables showing sprites */
    bool show_background : 1; /**< Enables/disables showing background */
    bool frame_changed : 1;   /**< Whether the VCE *just* went into VBlank (frame finished) */

    HuC6260* vce; /**< Video Color Encoder (HuC6260) state */

    HuC6270* vdc_a; /**< Primary Video Display Controller (HuC6270) state */

    HuC6270*
        vdc_b; /**< Secondary Video Display Controller (HuC6270) state (SuperGrafx exclusive) */
    // TODO: there's like a priority controller chip on the sgx too :sob:

} PCEGraphics;

/**
 * @brief Creates a new PC Engine graphics pipeline
 *
 * @param extended Whether to create a SuperGrafx pipeline (dual VDCs)
 *
 * @return The created graphics pipeline
 */
PCEGraphics* pce_gfx_create(bool extended);

/**
 * @brief Resets the provided graphics pipeline to a default state
 *
 * @param gfx The graphics pipeline to reset
 */
void pce_gfx_reset(PCEGraphics* gfx);

/**
 * @brief Ticks the provided graphics pipeline forward by the provided number of master clock cycles
 *
 * @param gfx The graphics pipeline to tick
 * @param clocks The number of clocks to advance the graphics pipeline by
 */
void pce_gfx_step(PCEGraphics* gfx, u16 clocks);

/**
 * @brief Destroys and frees the provided graphics pipeline
 *
 * @param gfx The graphics pipeline to destroy
 */
void pce_gfx_destroy(PCEGraphics* gfx);

/**
 * @brief Reads one byte of data from the graphics pipeline
 *
 * @note The first argument is a PCEGraphics* in a void* hat
 *
 * @see memory.h
 */
u8 pce_gfx_read(void* gfxptr, u32 addr);

/**
 * @brief Writes one byte of data to the graphics pipeline
 *
 * @note The first argument is a PCEGraphics* in a void* hat
 *
 * @see memory.h
 */
void pce_gfx_write(void* gfxptr, u32 addr, u8 value);

/**
 * @brief Attempts to get a new frame from the graphics pipeline's VCE chip.
 *
 * @details This will check if the VCE is currently available to get the frame buffer and whether a
 * new frame has been rendered, and if both conditions are true, the new frame data along with the
 * VCE's current output resolution will be copied to the provided PCE_GFXFrame pointer, and the
 * function will return true. Otherwise, no data gets copied or modified, and the function returns
 * false.
 *
 * @param gfx The graphics pipeline to get the frame from
 * @param frame_out_ptr The pointer to a PCE_GFXFrame struct to copy the data into.
 *
 * @return Whether there was a new frame that was copied
 */
bool pce_gfx_getframe(PCEGraphics* gfx, PCE_GFXFrame* frame);

#endif
