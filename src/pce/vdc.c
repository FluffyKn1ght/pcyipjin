#include "pce/vdc.h"
#include "memory.h"
#include "pce/vce.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DOTCYCLES_PER_CHARCYCLE 8

/*
 * TODO LIST:
 * - Correct(ish) memory access width emulation
 * - State machine and timing stuff
 * - Framebuffer output
 * - Emulator callbacks (interrupts, frame done, etc.)
 */

#define REG_WRITE(addr, reg, mask)                                                                 \
    case addr: {                                                                                   \
        reg = ((reg) & (0xFF >> (a0 * 8))) | (value << (a0 * 8));                                  \
        reg &= (mask);                                                                             \
        break;                                                                                     \
    }

#define VRAM_ADDR_MASK 0x7FFF
#define CR_MASK 0b1111111111111
#define RCR_MASK 0b111111111
#define BXR_MASK 0b111111111
#define BYR_MASK 0b11111111
#define MWR_MASK 0b11111111
#define DCR_MASK 0x1F
#define HSR_MASK 0b0111111100011111
#define HDR_MASK 0b0111111101111111
#define VPR_MASK 0b1111111100011111
#define VDR_MASK 0x01FF
#define VCR_MASK 0xFF

static VCEInput _vdc_render(VDC* vdc) {
    printf(FILEPOS "_vdc_render: stub\n");
    return (VCEInput){};
}

static VCEInput _vdc_step(VDC* vdc) {
    // whatever deity there is, may you forgive me for my sins...
    // - fluffykn1ght

    if (vdc->state == VDC_STATE_FRAMESTART) {
        vdc->state = VDC_STATE_VBLANK;
        vdc->scanline_count = vdc->vpr.vsw + 1 + vdc->vcr + 1 + vdc->vpr.vds + 2;
    }

    if (vdc->state == VDC_STATE_BURST) {
        if (vdc->charcycle_count <= 0) {
            vdc->scanline_count--;
            vdc->charcycle_count =
                vdc->hsr.hsw + 1 + vdc->hsr.hds + 1 + vdc->hdr.hdw + 1 + vdc->hdr.hde + 1;
        }

        if (vdc->scanline_count <= 0) {
            vdc->state = VDC_STATE_FRAMESTART;
        }
        return (VCEInput){.blank = true, .spbg = 1, .color = 0};
    }

    if (vdc->state == VDC_STATE_VBLANK) {
        if (vdc->charcycle_count <= 0) {
            vdc->scanline_count--;
            vdc->charcycle_count =
                vdc->hsr.hsw + 1 + vdc->hsr.hds + 1 + vdc->hdr.hdw + 1 + vdc->hdr.hde + 1;
        }

        if (vdc->scanline_count <= 0) {
            if (!vdc->_render_sprites && !vdc->_render_bg) {
                // fblank joined the chat
                vdc->state = VDC_STATE_BURST;
                vdc->scanline_count = vdc->vdw + 1;
            } else {
                vdc->state = VDC_STATE_HSYNC;
                vdc->charcycle_count = vdc->hsr.hsw + 1;
                vdc->scanline_count = vdc->vdw + 1;
            }
        }
    }

    switch (vdc->state) {
    case VDC_STATE_HSYNC: {
        if (vdc->charcycle_count <= 0) {
            vdc->state = VDC_STATE_HBLANK_BEFORE;
            vdc->charcycle_count = vdc->hsr.hds + 1;
        }
        break;
    }
    case VDC_STATE_HBLANK_BEFORE: {
        if (vdc->charcycle_count <= 0) {
            vdc->state = VDC_STATE_RENDER;
            vdc->charcycle_count = vdc->hdr.hdw + 1;
        }
        break;
    }
    case VDC_STATE_RENDER: {
        if (vdc->charcycle_count <= 0) {
            vdc->state = VDC_STATE_HBLANK_AFTER;
            vdc->charcycle_count = vdc->hdr.hde + 1;
            break;
        }
        return _vdc_render(vdc);
    }
    case VDC_STATE_HBLANK_AFTER: {
        if (vdc->charcycle_count <= 0) {
            vdc->charcycle_count = vdc->hsr.hsw + 1;
            vdc->scanline_count--;

            if (vdc->scanline_count > 0) {
                vdc->state = VDC_STATE_HSYNC;
            } else {
                vdc->state = VDC_STATE_FRAMESTART;
            }
        }
        break;
    }
    default: {
        printf(FILEPOS "unknown vdc state %u\n", vdc->state);
        assert(false);
        break;
    }
    }

    return (VCEInput){.blank = true, .spbg = 1, .color = 0};
}

VDC* vdc_create() {
    VDC* vdc = calloc(1, sizeof(VDC));

    vdc->vram = malloc(sizeof(VDC_VRAM));
    vdc->satb = malloc(sizeof(VDC_SATB));

    vdc->bg_visible = true;
    vdc->sprites_visible = true;

    return vdc;
}

void vdc_destroy(VDC* vdc) {
    free(vdc->satb);
    free(vdc->vram);
    free(vdc);
}

void vdc_reset(VDC* vdc) {
    vdc->reg = 0;
    vdc->status.byte = 0;

    memset(vdc->vram, 0xFF, sizeof(VDC_VRAM));
    memset(vdc->satb, 0xFF, sizeof(VDC_SATB));

    vdc->mawr = 0;
    vdc->marr = 0;
    vdc->cr.word = 0;
    vdc->rcr = 0;
    vdc->bxr = 0;
    vdc->byr = 0;
    vdc->mwr.word = 0;
    vdc->dcr.word = 0;
    vdc->sour = 0;
    vdc->desr = 0;
    vdc->lenr = 0;
    vdc->dvssr = 0;
    vdc->hsr.word = 0;
    vdc->hdr.word = 0;
    vdc->vpr.word = 0;
    vdc->vdw = 0;
    vdc->vcr = 0;

    vdc->vram_dma_state = VDC_DMA_IDLE;
    vdc->satb_dma_state = VDC_DMA_IDLE;

    vdc->irq = false;

    vdc->_bgscroll_x = 0;
    vdc->_bgscroll_y = 0;
    vdc->_vscreen_size_x = 0;
    vdc->_vscreen_size_y = 0;
    vdc->_render_sprites = false;
    vdc->_render_bg = false;
    vdc->_use_alt_cg = false;

    vdc->state = VDC_STATE_BURST;
    vdc->charcycle_count = 0;
    vdc->dotcycle_count = 0;
}

u8 vdc_read(void* vdcptr, MemoryAccess access, u16 addr) {
    VDC* vdc = (VDC*)vdcptr;

    u16 internal_addr = (addr - MEM_HUC6270_START) & 0x3;

    u8 a0 = internal_addr & 0x1;
    u8 a1 = internal_addr & 0x2;

    if (a1 == 0) {
        if (a0 == 1) {
            return 0;
        } else {
            return vdc->status.byte;
        }
    } else {
        if (vdc->reg == VDC_REG_VWR_VRR) {
            // TODO
            return MEM_UNDEFINED;
        } else {
            return MEM_UNDEFINED;
        }
    }

    return MEM_UNDEFINED;
}

void vdc_write(void* vdcptr, MemoryAccess access, u16 addr, u8 value) {
    VDC* vdc = (VDC*)vdcptr;

    u16 internal_addr = (addr - MEM_HUC6270_START) & 0x3;

    u8 a0 = internal_addr & 0x1;
    u8 a1 = internal_addr & 0x2;

    if (a1 == 0) {
        if (a0 == 0) {
            vdc->reg = value & 0x1F;
        }
    } else {
        switch (vdc->reg) {
            REG_WRITE(VDC_REG_MAWR, vdc->mawr, VRAM_ADDR_MASK);
            REG_WRITE(VDC_REG_MARR, vdc->marr, VRAM_ADDR_MASK);
        case VDC_REG_VWR_VRR: {
            // TODO: write vram and bump mawr
            break;
        }
            REG_WRITE(VDC_REG_CR, vdc->cr.word, CR_MASK);
            REG_WRITE(VDC_REG_RCR, vdc->rcr, RCR_MASK);
            REG_WRITE(VDC_REG_BXR, vdc->bxr, BXR_MASK);
            REG_WRITE(VDC_REG_BYR, vdc->byr, BYR_MASK);
            REG_WRITE(VDC_REG_MWR, vdc->mwr.word, MWR_MASK);
            REG_WRITE(VDC_REG_DCR, vdc->dcr.word, DCR_MASK);
            REG_WRITE(VDC_REG_SOUR, vdc->sour, VRAM_ADDR_MASK);
            REG_WRITE(VDC_REG_DESR, vdc->desr, VRAM_ADDR_MASK);
            REG_WRITE(VDC_REG_HSR, vdc->hsr.word, HSR_MASK);
            REG_WRITE(VDC_REG_HDR, vdc->hdr.word, HDR_MASK);
            REG_WRITE(VDC_REG_VPR, vdc->vpr.word, VPR_MASK);
            REG_WRITE(VDC_REG_VDR, vdc->vdw, VDR_MASK);
            REG_WRITE(VDC_REG_VCR, vdc->vcr, VCR_MASK);

        case VDC_REG_LENR: {
            vdc->lenr = (vdc->lenr & (0xFF >> (a0 * 8))) | (value << (a0 * 8));
            if (a0 == 1) {
                vdc->vram_dma_state = VDC_DMA_ARMED;
            }

            break;
        }

        case VDC_REG_DVSSR: {
            vdc->dvssr = (vdc->dvssr & (0xFF >> (a0 * 8))) | (value << (a0 * 8));
            if (a0 == 1) {
                vdc->satb_dma_state = VDC_DMA_ARMED;
            }

            break;
        }
        }
    }
}

VCEInput vdc_tick(void* vdcptr) {
    VDC* vdc = (VDC*)vdcptr;

    VCEInput out = _vdc_step(vdc);

    vdc->dotcycle_count--;
    if (vdc->dotcycle_count <= 0) {
        vdc->charcycle_count--;
        vdc->dotcycle_count = DOTCYCLES_PER_CHARCYCLE;
    }

    return out;
}
