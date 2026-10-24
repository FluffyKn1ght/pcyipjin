#include "emulator.h"
#include "memory.h"
#include "pce/cpu.h"
#include "pce/graphics.h"
#include <stdio.h>
#include <stdlib.h>

static void _emu_cpu_sync(void* emuptr) {
    PCEEmulator* emu = (PCEEmulator*)emuptr;

    u16 clocks = emu->cpu->high_speed ? CPU_CLOCKDIV_HIGH : CPU_CLOCKDIV_LOW;

    emu->cycles += clocks;

    pce_gfx_step(emu->gfx, clocks);
}

PCEEmulator* pce_emu_create(PCEConsoleFeatures features) {
    PCEEmulator* emu = calloc(1, sizeof(PCEEmulator));

    emu->features = features;

    emu->cpu = calloc(1, sizeof(HuC6280));
    emu->mem = pce_mem_create();

    emu->gfx = pce_gfx_create(features.supergrafx);

    return emu;
}

void pce_emu_destroy(PCEEmulator* emu) {
    pce_mem_free(emu->mem);
    pce_gfx_destroy(emu->gfx);
    free(emu->cpu);

    free(emu);
}

void pce_emu_reset(PCEEmulator* emu, bool hard) {
    pce_gfx_reset(emu->gfx);
    pce_cpu_reset(emu->cpu, emu->mem, _emu_cpu_sync, (void*)emu);

    emu->cycles = 0;
}

void pce_emu_step(PCEEmulator* emu) { pce_cpu_step(emu->cpu, emu->mem, _emu_cpu_sync, (void*)emu); }
