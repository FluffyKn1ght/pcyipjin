#include "pce/vdc.h"
#include "memory.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

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

    // todo: reset everything else
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

        default: {
            printf(FILEPOS "unknown vdc register $%02X\n", vdc->reg);
            assert(false);
            break;
        }
        }
    }
}

u8 vdc_step(void* vdcptr) {
    VDC* vdc = (VDC*)vdcptr;

    printf(FILEPOS "vdc_step: stub\n");

    return 0;
}
