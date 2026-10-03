#include "pce/vdc.h"
#include "memory.h"
#include <assert.h>
#include <stdio.h>

void vdc_reset(VDC* vdc) { printf(FILEPOS "vdc_reset: stub\n"); }

u8 vdc_read(void* vdcptr, MemoryAccess access, u16 addr) {
    printf(FILEPOS "vdc_read: stub\n");
    return MEM_UNDEFINED;
}

void vdc_write(void* vdcptr, MemoryAccess access, u16 addr, u8 value) {
    printf(FILEPOS "vdc_write: stub\n");
}

void vdc_step(void* vdcptr) {
    VDC* vdc = (VDC*)vdcptr;

    printf(FILEPOS "vdc_step: stub\n");
}
