#include "pce/graphics.h"
#include "memory.h"
#include "pce/vce.h"
#include "pce/vdc.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

PCEGraphics* pce_gfx_create(bool extended) {
    PCEGraphics* gfx = calloc(1, sizeof(PCEGraphics));

    gfx->extended = extended;

    gfx->vce = pce_vce_create();

    gfx->vdc_a = pce_vdc_create();

    if (extended) {
        gfx->vdc_b = pce_vdc_create();

        // TODO: Create a VPC as well
    }

    pce_gfx_reset(gfx);

    return gfx;
}

void pce_gfx_reset(PCEGraphics* gfx) {
    gfx->irq1 = false;
    gfx->frame_changed = false;

    pce_vce_reset(gfx->vce);

    pce_vdc_reset(gfx->vdc_a);

    if (gfx->extended) {
        pce_vdc_reset(gfx->vdc_b);

        // TODO: Reset the VPC
    }
}

void pce_gfx_step(PCEGraphics* gfx, u16 clocks) {
    while (clocks--) {
        if (pce_vce_tick(gfx->vce)) {
            PCE_VDCOutput vdc_a_out, vdc_b_out, vdc_out;

            if (gfx->extended) {
                vdc_a_out = pce_vdc_step(gfx->vdc_a);

                // TODO: slave mode
                // gfx->vdc_b->pin_hsync = gfx->vdc_a->pin_hsync;
                // gfx->vdc_b->pin_vsync = gfx->vdc_a->pin_vsync;
                vdc_b_out = pce_vdc_step(gfx->vdc_b);

                // TODO: Resolve priority via VPC
            } else {
                gfx->vdc_a->cr.ex = VDC_EXSYNC_OFF;

                vdc_a_out = pce_vdc_step(gfx->vdc_a);
                vdc_out = vdc_a_out;
            }

            if (!gfx->show_sprites) {
                vdc_out.spr = 0;
                vdc_out.spbg = false;
            }

            if (!gfx->show_background) {
                vdc_out.bg = 0;
                vdc_out.spbg = true;
            }

            gfx->frame_changed |= pce_vce_render(gfx->vce, vdc_out);
        }
    }
}

void pce_gfx_destroy(PCEGraphics* gfx) {
    pce_vce_destroy(gfx->vce);
    pce_vdc_destroy(gfx->vdc_a);

    if (gfx->extended) {
        pce_vdc_destroy(gfx->vdc_b);
        // TODO: Destroy the VPC
    }

    free(gfx);
}

// TODO: map graphics to 0x1FE000-0x1FE4FF

u8 pce_gfx_read(void* gfxptr, u32 addr) {
    PCEGraphics* gfx = (PCEGraphics*)gfxptr;

    if (addr >= MEM_HUC6260_START && addr <= MEM_HUC6260_END) {
        return pce_vce_read(gfx->vce, addr & 0xFF);
    } else if (addr >= MEM_HUC6270_START && addr <= MEM_HUC6270_END) {
        if (gfx->extended) {
            // TODO
            return MEM_UNDEFINED;
        } else {
            return pce_vdc_read(gfx->vdc_a, addr & 0xFF);
        }
    }

    return 0xFF;
}

void pce_gfx_write(void* gfxptr, u32 addr, u8 value) {
    PCEGraphics* gfx = (PCEGraphics*)gfxptr;

    if (addr >= MEM_HUC6260_START && addr <= MEM_HUC6260_END) {
        pce_vce_write(gfx->vce, addr & 0xFF, value);
        return;
    } else if (addr >= MEM_HUC6270_START && addr <= MEM_HUC6270_END) {
        if (gfx->extended) {
            // TODO
        } else {
            pce_vdc_write(gfx->vdc_a, addr & 0xFF, value);
        }

        return;
    }
}

bool pce_gfx_getframe(PCEGraphics* gfx, PCE_GFXFrame* frame) {
    if (!gfx->vce->busy && gfx->frame_changed) {
        frame->width = gfx->vce->frame_size_x;
        frame->height = gfx->vce->frame_size_y;
        memcpy(frame->pixels, gfx->vce->frame_out, sizeof(frame->pixels));

        gfx->frame_changed = false;

        return true;
    } else {
        return false;
    }
}
