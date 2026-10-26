#include "pce/vce.h"
#include "memory.h"
#include <stdlib.h>
#include <string.h>

#define REGISTER_COUNT 3

#define REG_CR 0
#define REG_CTA 1
#define REG_CTRW 2

#define WRITE_WORD(dest, half, value)                                                              \
    dest = (((dest) << (8 - ((half) * 8))) | ((value) << ((half) * 8)) & 0xFFFF)

const u16 CLOCK_MODULOS[4] = {4, 3, 2, 2};

static inline RGB888 _grb333_to_rgb888(GRB333 grb333) {
    return (RGB888){.r = grb333.r << 5, .g = grb333.g << 5, .b = grb333.b << 5};
}

HuC6260* pce_vce_create() {
    HuC6260* vce = malloc(sizeof(HuC6260));

    vce->frame_out = calloc(VCE_MAX_X_SCREEN_SIZE * VCE_VDW_SCANLINE_COUNT, sizeof(RGB888));
    vce->cram = calloc(512, sizeof(GRB333));

    pce_vce_reset(vce);

    return vce;
}

void pce_vce_destroy(HuC6260* vce) {
    free(vce->cram);
    free(vce->frame_out);
    free(vce);
}

void pce_vce_reset(HuC6260* vce) {
    vce->clock_modulo = CLOCK_MODULOS[0];

    vce->clock_counter = vce->clock_modulo;

    vce->cta = 0;

    vce->current_pos_x = 0;
    vce->current_pos_y = 0;

    vce->latched_vsync = false;
    vce->latched_hsync = false;

    // TODO: randomize uninitialized memory?
    memset(vce->cram, 0, sizeof(GRB333) * 512);

    memset(vce->frame_out, 0, sizeof(GRB333) * 512);
    vce->busy = false;
    vce->frame_size_x = 0;
    vce->frame_size_y = 0;
}

bool pce_vce_tick(HuC6260* vce) {
    if (vce->clock_counter-- <= 0) {
        vce->clock_counter = vce->clock_modulo;
        return true;
    }

    return false;
}

bool pce_vce_render(HuC6260* vce, PCE_VDCOutput vdc_out) {
    bool just_went_into_vblank = vdc_out.vsync && !vce->latched_vsync;
    bool just_went_into_hblank = vdc_out.hsync && !vce->latched_hsync;

    vce->latched_hsync = vdc_out.hsync;
    vce->latched_vsync = vdc_out.vsync;

    if (just_went_into_hblank) {
        vce->current_pos_x = 0;
        vce->current_pos_y++;
    }

    if (just_went_into_vblank) {
        vce->current_pos_x = 0;
        vce->current_pos_y = 0;
    }

    if (!vdc_out.hsync && !vdc_out.vsync) {
        GRB333 grb333_color = vce->cram[vdc_out.spbg ? vdc_out.spr : vdc_out.bg];

        u32 current_offset = vce->current_pos_x + vce->current_pos_y * VCE_VDW_SCANLINE_COUNT;
        if (current_offset < 512 * VCE_VDW_SCANLINE_COUNT) {
            vce->frame_out[current_offset] = _grb333_to_rgb888(grb333_color);
            vce->current_pos_x++;
        }
    }

    if (vce->current_pos_x > vce->frame_size_x) {
        vce->frame_size_x = vce->current_pos_x;
    }
    if (vce->current_pos_x > vce->frame_size_y) {
        vce->frame_size_y = vce->current_pos_y;
    }

    return just_went_into_vblank;
}

u8 pce_vce_read(HuC6260* vce, u8 addr) {
    u8 reg_no = (addr >> 1) % REGISTER_COUNT;
    u8 reg_half = addr & 1;

    if (addr == REG_CTRW) {
        u8 value = vce->cram[vce->cta].word & (0xFF << (reg_half * 8));

        if (reg_half) {
            vce->cta++;
        }

        return value;
    }

    return MEM_UNDEFINED;
}

void pce_vce_write(HuC6260* vce, u8 addr, u8 value) {
    u8 reg_no = (addr >> 1) % REGISTER_COUNT;
    u8 reg_half = addr & 1;

    switch (reg_no) {
    case REG_CR: {
        if (!reg_half) {
            u8 clock_setting = value & 0b11;
            vce->clock_modulo = CLOCK_MODULOS[clock_setting];
        }

        break;
    }
    case REG_CTA: {
        WRITE_WORD(vce->cta, reg_half, value);
        break;
    }
    case REG_CTRW: {
        WRITE_WORD(vce->cram[vce->cta].word, reg_half, value);

        if (reg_half) {
            vce->cta++;
        }
    }
    }
}
