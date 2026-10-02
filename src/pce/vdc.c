#include "pce/vdc.h"
#include <assert.h>
#include <stdio.h>

void vdc_reset(VDC* vdc) { printf(FILEPOS "vdc_reset: stub\n"); }

void vdc_write(VDC* vdc, u8 addr, u8 value) {
    assert(addr >= 0 && addr <= 2);

    printf(FILEPOS "vdc_write: stub\n");
}
