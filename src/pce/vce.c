#include "pce/vce.h"
#include "memory.h"
#include <assert.h>
#include <stdcountof.h>
#include <stdio.h>
#include <string.h>

#define REG_CR 0  // Control Register
#define REG_CTA 1 // Color Table Address
#define REG_CTW 2 // Color Table Write
#define REG_CTR 2 // Color Table Read

// note: the reason this is an array is that there was something written about 10MHz mode
const u16 CLOCK_MODULOS[2] = {VCE_CLOCKDIV_5MHZ, VCE_CLOCKDIV_7MHZ};

static void _vce_step(VCE* vce, VCEInput input) { printf(FILEPOS "_vce_step: stub\n"); }

void vce_reset(VCE* vce) {
    memset(vce->cram.byte, 0, sizeof(vce->cram.byte));
    vce->clock_modulo = CLOCK_MODULOS[0];
    vce->clock_counter = vce->clock_modulo;
}

u8 vce_read(void* vceptr, MemoryAccess access, u32 addr) {
    VCE* vce = (VCE*)vceptr;

    u8 internal_addr = (addr - MEM_HUC6260_START) % 8;
    assert(internal_addr < 8);

    u8 reg = internal_addr / 2;
    u8 a0 = internal_addr % 2;

    if (reg == REG_CTR) {
        u8 value = vce->cram.byte[vce->cta + a0];

        if (a0 == 1) {
            vce->cta++;
            vce->cta &= countof(vce->cram.color) - 1;
        }

        return value;
    } else {
        return MEM_UNDEFINED;
    }
}

void vce_write(void* vceptr, MemoryAccess access, u32 addr, u8 value) {
    VCE* vce = (VCE*)vceptr;

    u8 internal_addr = (addr - MEM_HUC6260_START) % 8;
    assert(internal_addr < 8);

    u8 reg = internal_addr / 2;
    u8 a0 = internal_addr % 2;

    switch (reg) {
    case REG_CR: {
        if (a0 == 0) {
            vce->clock_counter = 0;
            vce->clock_modulo = CLOCK_MODULOS[value & 0x1];
        }

        break;
    }
    case REG_CTA: {
        if (a0 == 1) {
            vce->cta = ((value << 8) & 0x1) | (vce->cta & 0xFF);
        } else {
            vce->cta = (vce->cta & 0xFF00) | value;
        }

        break;
    }
    case REG_CTW: {
        if (a0 == 1) {
            value &= 0x1;
        }

        vce->cram.byte[vce->cta + a0] = value;

        if (a0 == 1) {
            vce->cta++;
            vce->cta &= countof(vce->cram.color) - 1;
        }

        break;
    }
    }
}

void vce_tick(VCE* vce, u16 clocks, VCEInput (*vdc_tick_func)(void*), void* vdc_tick_arg) {
    vce->clock_counter -= clocks;
    while (vce->clock_counter <= 0) {
        vce->clock_counter += vce->clock_modulo;
        _vce_step(vce, vdc_tick_func(vdc_tick_arg));
    }
}
