#include "pce/vdc.h"
#include "memory.h"
#include <assert.h>
#include <stdio.h>

void vdc_reset(VDC* vdc) {
    vdc->reg = 0;
    vdc->status.byte = 0;
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
        default: {
            printf(FILEPOS "unknown vdc register $%02X\n", vdc->reg);
            assert(false);
            break;
        }
        }
    }
}

void vdc_step(void* vdcptr) {
    VDC* vdc = (VDC*)vdcptr;

    printf(FILEPOS "vdc_step: stub\n");
}
