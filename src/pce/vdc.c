#include "pce/vdc.h"
#include "pce/vce.h"
#include "utils/queue.h"
#include <stdcountof.h>
#include <stdlib.h>
#include <string.h>

#define CHARCYCLES_PER_LINE                                                                        \
    ((vdc->_latched_hsw + 1) + (vdc->_latched_hds + 1) + (vdc->_latched_hdw + 1) +                 \
     (vdc->_latched_hde + 1))

#define UNDEFINED 0xFFFF
#define REG04_CONTENT 42069

#define VISIBLE_TOPLEFT_X 32
#define VISIBLE_TOPLEFT_Y 64

#define VRAM_MASK 0b0111111111111111

#define WRITE_WORD(dest, half, value)                                                              \
    dest = ((dest) << (1 - ((half) * 8))) | ((value) << ((half) * 8))

typedef enum : u8 {
    VRAMCYCLE_NONE = 0, /**< Do nothing */
    VRAMCYCLE_CPU, /**< Flush/update VRAM access buffer for VWRW register, clearing the BSY status
                      flag */
    VRAMCYCLE_BAT, /**< Access Background Attribute Table */
    VRAMCYCLE_CG0, /**< Access CG0 (bitplanes 0 and 1) */
    VRAMCYCLE_CG1, /**< Access CG1 (bitplanes 2 and 3) */
    VRAMCYCLE_CGN, /**< Access CG0 *or* CG1 (only one!), depending on the MWR register's CM
                      bit */
} _VDC_VRAMAccessCycle;

const u16 PCE_VDC_VSCREEN_SIZES[VDC_SCREEN_COUNT][2] = {
    [VDC_SCREEN_32_32] = {32, 32},   [VDC_SCREEN_64_32] = {64, 32},
    [VDC_SCREEN_128_32] = {128, 32}, [VDC_SCREEN_128_32_] = {128, 32},
    [VDC_SCREEN_32_64] = {32, 64},   [VDC_SCREEN_64_64] = {64, 64},
    [VDC_SCREEN_128_64] = {128, 64}, [VDC_SCREEN_128_64_] = {128, 64}};

// first element is amount of dot cycles per access, other elements are _VDC_VRAMAccessCycle entries
static const _VDC_VRAMAccessCycle VRAM_ACCESS_CYCLES[VDC_VM_COUNT][9] = {
    [VDC_VM_1] = {1, VRAMCYCLE_CPU, VRAMCYCLE_BAT, VRAMCYCLE_CPU, VRAMCYCLE_NONE, VRAMCYCLE_CPU,
                  VRAMCYCLE_CG0, VRAMCYCLE_CPU, VRAMCYCLE_CG1},
    [VDC_VM_2A] = {2, VRAMCYCLE_BAT, VRAMCYCLE_CPU, VRAMCYCLE_BAT, VRAMCYCLE_CPU},
    [VDC_VM_2B] = {2, VRAMCYCLE_BAT, VRAMCYCLE_CPU, VRAMCYCLE_BAT, VRAMCYCLE_CPU},
    [VDC_VM_4] = {4, VRAMCYCLE_BAT, VRAMCYCLE_CGN}};

static const u8 SPRITE_CGY_HEIGHTS[VDC_CGY_COUNT] = {
    [VDC_CGY_OFF] = 16, [VDC_CGY_2CGY] = 32, [VDC_CGY_INVALID] = 16, [VDC_CGY_4CGY] = 64};

static const u8 SG_ACCESS_WIDTHS[VDC_SM_COUNT] = {
    [VDC_SM_1] = 1, [VDC_SM_2] = 2, [VDC_SM_2_LSB] = 2, [VDC_SM_4] = 4};

static inline u16 _get_bat_addr_for_pixel(HuC6270* vdc, u16 x, u16 y) {
    u16 coarse_x = x / 8;
    u16 coarse_y = y / 8;

    coarse_x %= vdc->_latched_vscr_width;
    coarse_y %= vdc->_latched_vscr_height;

    return coarse_x + coarse_y * vdc->_latched_vscr_width;
}

static inline u8 _get_bit(u16 byte, u8 bit) { return (byte & (0x1 << bit)) >> bit; }

static void _vdc_access_vram_cg(HuC6270* vdc) {
    _VDC_VRAMAccessCycle cur_vram_cycle =
        VRAM_ACCESS_CYCLES[vdc->_latched_vm]
                          [vdc->_cg_access_dotcycles % VRAM_ACCESS_CYCLES[vdc->_latched_vm][0]];

    vdc->_cg_access_dotcycles++;
    vdc->_cg_access_dotcycles %= 8 / VRAM_ACCESS_CYCLES[vdc->_latched_vm][0];

    switch (cur_vram_cycle) {
    case VRAMCYCLE_NONE: {
        break;
    }
    case VRAMCYCLE_CPU: {
        if (vdc->sr.bsy) {
            vdc->sr.bsy = false;

            if (vdc->vram_access_is_write) {
                vdc->vram[vdc->mawr & VRAM_MASK] = vdc->vram_write_buf;
            } else {
                vdc->vram_read_buf = vdc->vram[vdc->marr & VRAM_MASK];
            }
        }
        break;
    }
    case VRAMCYCLE_BAT: {
        vdc->_temp_bat.word = vdc->vram[_get_bat_addr_for_pixel(vdc, vdc->_latched_bgx + vdc->pixel,
                                                                vdc->_latched_bgy + vdc->scanline) &
                                        VRAM_MASK];
        break;
    }
    case VRAMCYCLE_CG0: {
        vdc->_temp_cg[0] =
            vdc->vram[((vdc->_temp_bat.charcode << 4) + ((vdc->_latched_bgy + vdc->scanline) % 8)) &
                      VRAM_MASK];
        break;
    }
    case VRAMCYCLE_CG1: {
        vdc->_temp_cg[1] = vdc->vram[(((vdc->_temp_bat.charcode + 1) << 4) +
                                      ((vdc->_latched_bgy + vdc->scanline) % 8)) &
                                     VRAM_MASK];
        break;
    }
    case VRAMCYCLE_CGN: {
        vdc->_temp_cg[vdc->_latched_cm] =
            vdc->vram[(((vdc->_temp_bat.charcode + vdc->_latched_cm) << 4) +
                       ((vdc->_latched_bgy + vdc->scanline) % 8)) &
                      VRAM_MASK];
        break;
    }
    }
}

static void _vdc_access_vram_sg(HuC6270* vdc) {
    PCE_VDCInternalSprite* ispr = vdc->_line_sprites + vdc->_sprite_sgload_progress;

    if (!ispr->populated) {
        return;
    }

    if (ispr->bitplanes_loaded >= 4) {
        vdc->_sprite_sgload_progress++;
        _vdc_access_vram_sg(vdc);
        return;
    }

    vdc->_sg_access_dotcycles++;
    vdc->_sg_access_dotcycles %= 8;

    u8 cycles_per_access = SG_ACCESS_WIDTHS[vdc->_latched_sm];
    if (vdc->_sg_access_dotcycles % cycles_per_access) {
        return;
    }

    u8 height = SPRITE_CGY_HEIGHTS[ispr->cgy];

    u8 bitplane_to_load = vdc->_sg_access_dotcycles % cycles_per_access % 8;
    if (vdc->_latched_sm == VDC_SM_2_LSB) {
        bitplane_to_load += (ispr->sg_sel << 1);
    } else {
        bitplane_to_load += (vdc->_sm_mode4_current_sg << 1);
    }

    u8 cur_sprite_scanline = (vdc->scanline - ispr->ypos);

    u8 cur_sprite_line =
        ispr->yflip ? SPRITE_CGY_HEIGHTS[ispr->cgy] - cur_sprite_scanline : cur_sprite_scanline;
    u8 y_offset = cur_sprite_line % 16;

    u8 pattern_cgy_bits = height == 16 ? ispr->pattern & 0b110 : cur_sprite_line / 4;

    u16 bitplane_addr = ((ispr->pattern & 0b11111111001) << 6) | ((pattern_cgy_bits & 0b110) << 6) |
                        (bitplane_to_load << 4) | (y_offset & 0xF);

    ispr->bitplanes[bitplane_to_load] = vdc->vram[bitplane_addr & VRAM_MASK];

    if (vdc->_latched_sm == VDC_SM_2_LSB) {
        if (ispr->bitplanes_loaded >= 2) {
            vdc->_sprite_sgload_progress++;
        }
    } else if (vdc->_latched_sm == VDC_SM_4) {
        if (ispr->bitplanes_loaded == 2) {
            vdc->_sm_mode4_current_sg ^= true;
        }
    } else {
        if (ispr->bitplanes_loaded >= 4) {
            vdc->_sprite_sgload_progress++;
        }
    }
}

static void _vdc_render_px_to_fifo(HuC6270* vdc) {
    u8 ch0 = vdc->_temp_cg[0] & 0xFF;
    u8 ch1 = (vdc->_temp_cg[0] & 0xFF00) >> 8;
    u8 ch2 = vdc->_temp_cg[1] & 0xFF;
    u8 ch3 = (vdc->_temp_cg[1] & 0xFF00) >> 8;

    u8 bg_out = _get_bit(ch0, vdc->pixel % 8) | (_get_bit(ch1, vdc->pixel % 8) << 1) |
                (_get_bit(ch2, vdc->pixel % 8) << 2) | (_get_bit(ch3, vdc->pixel % 8) << 3) |
                (vdc->_temp_bat.cgcolor << 4);
    u8 spr_out = 0;
    bool spbg = false;
    bool sprite_rendered = false;
    bool has_sprite_0_pixel = false;

    for (int i = 0; i < 16; i++) {
        PCE_VDCInternalSprite* spr = vdc->_line_sprites + i;
        if (spr->populated) {
            if (vdc->pixel >= spr->xpos) {
                u8 spr_x = spr->xflip ? 16 - (vdc->pixel - spr->xpos) : (vdc->pixel - spr->xpos);
                u8 spr_pixel = _get_bit(spr->bitplanes[0], spr_x) |
                               (_get_bit(spr->bitplanes[1], spr_x) << 1) |
                               (_get_bit(spr->bitplanes[2], spr_x) << 2) |
                               (_get_bit(spr->bitplanes[3], spr_x) << 3) | (spr->spr_color << 4);

                spbg = spr->spbg | !bg_out;

                if ((spr_pixel & 0xF) != 0) {
                    if (spr->is_sprite_0) {
                        has_sprite_0_pixel = spbg;
                    } else {
                        if (has_sprite_0_pixel && spbg) {
                            vdc->sr.cr = true;
                        }
                    }
                }

                if (!spr_out) {
                    spr_out = spr_pixel;
                }

                sprite_rendered = true;
            }

        } else {
            break;
        }
    }

    PCE_VDCOutput vdc_out = (PCE_VDCOutput){.bg = bg_out, .spr = spr_out, .spbg = spbg};

    queue_enqueue(vdc->_pixel_fifo, &vdc_out);

    vdc->pixel++;
    vdc->pixel %= VCE_MAX_X_SCREEN_SIZE;
}

static void _vdc_evaluate_sprites(HuC6270* vdc) {
    // this is not cycle accurate at all btw qwq

    // TODO: CGX

    u8 sprites_evaluated = 0;
    for (int i = 0; i < VDC_SPRITE_COUNT; i++) {
        PCE_VDCSprite sprite = vdc->satb[i];
        if (vdc->scanline >= sprite.y - VISIBLE_TOPLEFT_Y) {
            u8 height = SPRITE_CGY_HEIGHTS[sprite.cgy];

            if (vdc->scanline < (sprite.y - VISIBLE_TOPLEFT_Y) + height) {
                if (sprites_evaluated >= countof(vdc->_line_sprites)) {
                    vdc->sr.or = true;
                    break;
                }

                vdc->_line_sprites[sprites_evaluated].populated = true;
                vdc->_line_sprites[sprites_evaluated].spbg = sprite.spbg;
                vdc->_line_sprites[sprites_evaluated].is_sprite_0 = i == 0;
                vdc->_line_sprites[sprites_evaluated].spr_color = sprite.color;
                vdc->_line_sprites[sprites_evaluated].sg_sel = sprite.pc & 0x1;
                vdc->_line_sprites[sprites_evaluated].pattern = sprite.pc & (~0x1);
                vdc->_line_sprites[sprites_evaluated].xflip = sprite.xflip;
                vdc->_line_sprites[sprites_evaluated].yflip = sprite.yflip;
                vdc->_line_sprites[sprites_evaluated].xpos = sprite.x;
                vdc->_line_sprites[sprites_evaluated].ypos = sprite.y;

                sprites_evaluated++;
            }
        }
    }
}

static u16 _vdc_read_reg(HuC6270* vdc) {
    if (vdc->ar == VDC_REG_VWRW) {
        return vdc->vram_read_buf;
    }

    return UNDEFINED;
}

static void _vdc_write_reg(HuC6270* vdc, u8 a0, u16 value) {
    switch (vdc->ar) {
    case VDC_REG_MAWR: {
        WRITE_WORD(vdc->mawr, a0, value);
        break;
    }
    case VDC_REG_MARR: {
        if (a0) {
            vdc->vram_access_is_write = false;
            vdc->sr.bsy = true;
        }

        WRITE_WORD(vdc->marr, a0, value);
        break;
    }
    case VDC_REG_VWRW: {
        if (a0) {
            vdc->vram_access_is_write = true;
            vdc->sr.bsy = true;
        }
        WRITE_WORD(vdc->vram_write_buf, a0, value);
        break;
    }
    case VDC_REG_UNK03:
    case VDC_REG_UNK04: {
        break;
    }
    case VDC_REG_CR: {
        WRITE_WORD(vdc->cr.word, a0, value);
        break;
    }
    case VDC_REG_RCR: {
        WRITE_WORD(vdc->rcr, a0, value);
        break;
    }
    case VDC_REG_BXR: {
        WRITE_WORD(vdc->bxr, a0, value);
        break;
    }
    case VDC_REG_BYR: {
        WRITE_WORD(vdc->byr, a0, value);
        break;
    }
    case VDC_REG_MWR: {
        WRITE_WORD(vdc->rcr, a0, value);
        break;
    }
    case VDC_REG_HSR: {
        WRITE_WORD(vdc->hsr.word, a0, value);
        break;
    }
    case VDC_REG_HDR: {
        WRITE_WORD(vdc->hdr.word, a0, value);
        break;
    }
    case VDC_REG_VPR: {
        WRITE_WORD(vdc->vpr.word, a0, value);
        break;
    }
    case VDC_REG_VDW: {
        WRITE_WORD(vdc->vdw.word, a0, value);
        break;
    }
    case VDC_REG_VCR: {
        WRITE_WORD(vdc->vcr.word, a0, value);
        break;
    }
    case VDC_REG_DCR: {
        WRITE_WORD(vdc->dcr.word, a0, value);
        break;
    }
    case VDC_REG_SOUR: {
        WRITE_WORD(vdc->sour, a0, value);
        break;
    }
    case VDC_REG_DESR: {
        WRITE_WORD(vdc->desr, a0, value);
        break;
    }
    case VDC_REG_LENR: {
        WRITE_WORD(vdc->lenr, a0, value);
        break;
    }
    case VDC_REG_DVSSR: {
        WRITE_WORD(vdc->dvssr, a0, value);
        break;
    }
    default: {
        break;
    }
    }
}

HuC6270* pce_vdc_create() {
    HuC6270* vdc = malloc(sizeof(HuC6270));

    vdc->vram = calloc(32 * 1024, sizeof(u16));
    vdc->satb = calloc(64, sizeof(PCE_VDCSprite));

    vdc->_pixel_fifo = queue_create(sizeof(PCE_VDCOutput));
    queue_grow(vdc->_pixel_fifo, VCE_MAX_X_SCREEN_SIZE);

    pce_vdc_reset(vdc);

    return vdc;
}

void pce_vdc_destroy(HuC6270* vdc) {
    free(vdc->vram);
    free(vdc->satb);
    queue_free(vdc->_pixel_fifo);
    free(vdc);
}

void pce_vdc_reset(HuC6270* vdc) {
    // TODO: randomize uninitialized memory?
    memset(vdc->vram, 0, sizeof(u16) * 32 * 1024);
    memset(vdc->satb, 0, sizeof(PCE_VDCSprite) * VDC_SPRITE_COUNT);

    vdc->ar = 0;
    vdc->sr.word = 0;
    vdc->mawr = 0;
    vdc->marr = 0;
    vdc->cr.word = 0;
    vdc->rcr = 0;
    vdc->bxr = 0;
    vdc->byr = 0;
    vdc->mwr.word = 0;
    vdc->hsr.word = 0;
    vdc->hdr.word = 0;
    vdc->vpr.word = 0;
    vdc->vdw.word = 0;
    vdc->vcr.word = 0;
    vdc->dcr.word = 0;
    vdc->sour = 0;
    vdc->desr = 0;
    vdc->lenr = 0;
    vdc->dvssr = 0;

    vdc->vram_dma = false;
    vdc->satb_dma = VDC_SATBDMA_IDLE;

    vdc->irq1 = false;

    vdc->state = VDC_STATE_BURST;
    vdc->charcycle_count = 0;
    vdc->dotcycle_count = 0;
    vdc->_cg_access_dotcycles = 0;
    vdc->_sg_access_dotcycles = 0;

    vdc->scanline = 0;
    vdc->pixel = 0;

    vdc->_temp_bat = (PCE_VDC_BGAttrTableEntry){};
    vdc->_temp_cg[0] = 0;
    vdc->_temp_cg[1] = 0;

    queue_clear(vdc->_pixel_fifo);

    vdc->_latched_vm = vdc->mwr.vm;
    vdc->_latched_sm = vdc->mwr.sm;
    vdc->_latched_screen = vdc->mwr.screen;
    vdc->_latched_vscr_width = PCE_VDC_VSCREEN_SIZES[vdc->_latched_screen][0];
    vdc->_latched_vscr_height = PCE_VDC_VSCREEN_SIZES[vdc->_latched_screen][1];
    vdc->_latched_hsw = vdc->hsr.hsw;
    vdc->_latched_hds = vdc->hsr.hds;
    vdc->_latched_hdw = vdc->hdr.hdw;
    vdc->_latched_hde = vdc->hdr.hde;
    vdc->_latched_vsw = vdc->vpr.vsw;
    vdc->_latched_vds = vdc->vpr.vds;
    vdc->_latched_vdw = vdc->vdw.vdw;
    vdc->_latched_vcr = vdc->vcr.vcr;

    vdc->_latched_bb = vdc->cr.bb;
    vdc->_latched_bb = vdc->cr.sb;
    vdc->_latched_bgx = vdc->bxr;
    vdc->_latched_bgy = vdc->byr;
    vdc->_latched_cm = vdc->mwr.cm;
}

PCE_VDCOutput pce_vdc_step(HuC6270* vdc) {
    // TODO: DMA (VRAM and SATB)
    // TODO: Interrupts

    vdc->dotcycle_count--;

    if (vdc->dotcycle_count <= 0) {
        vdc->dotcycle_count = 8;
        vdc->charcycle_count--;
        _vdc_render_px_to_fifo(vdc);
    }

    if (vdc->state == VDC_STATE_HBLANK_HDS_PRIME || vdc->state == VDC_STATE_RENDER) {
        _vdc_access_vram_cg(vdc);
    } else if (vdc->state == VDC_STATE_HBLANK_HDE || vdc->state == VDC_STATE_HBLANK_HDS ||
               vdc->state == VDC_STATE_HSYNC) {
        if (!vdc->_sprite_eval_done) {
            _vdc_evaluate_sprites(vdc);
            vdc->_sprite_eval_done = true;
        }
        _vdc_access_vram_sg(vdc);
    }

    if (vdc->charcycle_count <= 0) {
        PCE_VDCState prev_state = vdc->state++;

        if (vdc->state >= VDC_STATE_COUNT) {
            vdc->state = 0;
        }

        if (vdc->state == VDC_STATE_VBLANK_VCR && vdc->scanline < vdc->_latched_vdw) {
            vdc->state = VDC_STATE_HSYNC;
        }

        if (prev_state == VDC_STATE_VBLANK_VDS && vdc->state == VDC_STATE_HSYNC && !vdc->cr.bb &&
            !vdc->cr.sb) {
            vdc->state = VDC_STATE_BURST;
        }

        if (prev_state == VDC_STATE_BURST && vdc->state == VDC_STATE_VBLANK_VDS) {
            vdc->state = VDC_STATE_VSYNC;
        }

        switch (vdc->state) {
        case VDC_STATE_BURST: {
            vdc->charcycle_count =
                ((vdc->_latched_vds + 2) + (vdc->_latched_vdw + 1) + vdc->_latched_vcr) *
                CHARCYCLES_PER_LINE;
            break;
        }
        case VDC_STATE_VBLANK_VDS: {
            vdc->charcycle_count = (vdc->_latched_vds + 2) * CHARCYCLES_PER_LINE;
            break;
        }
        case VDC_STATE_HSYNC: {
            if (prev_state == VDC_STATE_VBLANK_VDS) {
                vdc->_latched_vm = vdc->mwr.vm;
                vdc->_latched_sm = vdc->mwr.sm;
                vdc->_latched_screen = vdc->mwr.screen;
                vdc->_latched_vscr_width = PCE_VDC_VSCREEN_SIZES[vdc->_latched_screen][0];
                vdc->_latched_vscr_height = PCE_VDC_VSCREEN_SIZES[vdc->_latched_screen][1];
                vdc->_latched_hsw = vdc->hsr.hsw;
                vdc->_latched_hds = vdc->hsr.hds;
                vdc->_latched_hdw = vdc->hdr.hdw;
                vdc->_latched_hde = vdc->hdr.hde;
                vdc->_latched_vsw = vdc->vpr.vsw;
                vdc->_latched_vds = vdc->vpr.vds;
                vdc->_latched_vdw = vdc->vdw.vdw;
                vdc->_latched_vcr = vdc->vcr.vcr;
            }

            vdc->_latched_bb = vdc->cr.bb;
            vdc->_latched_bb = vdc->cr.sb;
            vdc->_latched_bgx = vdc->bxr;
            vdc->_latched_bgy = vdc->byr;
            vdc->_latched_cm = vdc->mwr.cm;

            // vdc->scanline++;

            vdc->charcycle_count = vdc->_latched_hsw + 1;
            break;
        }
        case VDC_STATE_HBLANK_HDS: {
            vdc->charcycle_count = vdc->_latched_hds - (VDC_PREFETCH_TILE_COUNT - 1);
            break;
        }
        case VDC_STATE_HBLANK_HDS_PRIME: {
            queue_clear(vdc->_pixel_fifo);
            vdc->pixel = 0;
            vdc->charcycle_count = 2;
            break;
        }
        case VDC_STATE_RENDER: {
            for (int _i = 0; _i < 8 - (vdc->_latched_bgx % 8); _i++) {
                queue_dequeue(vdc->_pixel_fifo, NULL);
            }

            vdc->charcycle_count = vdc->_latched_hdw + 1;
            break;
        }
        case VDC_STATE_HBLANK_HDE: {
            vdc->scanline++;
            vdc->_sprite_eval_done = false;
            vdc->_sprite_sgload_progress = 0;
            memset(vdc->_line_sprites, 0,
                   sizeof(PCE_VDCInternalSprite) * countof(vdc->_line_sprites));

            vdc->charcycle_count = vdc->_latched_hde + 1;
            break;
        }
        case VDC_STATE_VBLANK_VCR: {
            vdc->charcycle_count = vdc->_latched_vcr * CHARCYCLES_PER_LINE;
            break;
        }
        case VDC_STATE_VSYNC: {
            vdc->charcycle_count = (vdc->_latched_vsw + 1) * CHARCYCLES_PER_LINE;
            break;
        }
        }
    }

    bool hsync = vdc->state == VDC_STATE_HSYNC || vdc->state == VDC_STATE_HBLANK_HDS ||
                 vdc->state == VDC_STATE_HBLANK_HDE;
    bool vsync = vdc->state = VDC_STATE_VSYNC || vdc->state == VDC_STATE_VBLANK_VDS ||
                              vdc->state == VDC_STATE_VBLANK_VCR || vdc->state == VDC_STATE_BURST;

    if (vdc->state == VDC_STATE_RENDER) {
        PCE_VDCOutput out = {};
        queue_dequeue(vdc->_pixel_fifo, &out);

        return out;
    } else {
        return (PCE_VDCOutput){.spbg = true, .bg = 0, .spr = 0, .vsync = vsync, .hsync = hsync};
    }
}

u8 pce_vdc_read(HuC6270* vdc, u8 addr) {
    u8 reg_no = (addr >> 1) % 2;
    u8 reg_half = addr & 1;

    if (reg_no) {
        return _vdc_read_reg(vdc) >> (reg_half * 8);
    } else {
        vdc->sr.cr = false;
        vdc->sr.or = false;
        vdc->sr.rr = false;
        vdc->sr.ds = false;
        vdc->sr.dv = false;
        vdc->sr.vd = false;

        return vdc->sr.word >> (reg_half * 8);
    }
}

void pce_vdc_write(HuC6270* vdc, u8 addr, u8 value) {
    u8 reg_no = (addr >> 1) % 2;
    u8 reg_half = addr & 1;

    if (reg_no) {
        _vdc_write_reg(vdc, reg_half, value);
    }
}
