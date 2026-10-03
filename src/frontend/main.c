#include "emulator.h"
#include "pce/memory.h"
#include "pce/testmem.h"
#include <stdcountof.h>
#include <stdlib.h>
#include <string.h>
#ifndef _SHARED

const u8 MEMDATA[10] = {0x0A};

int argc;
char** argv;

int main(int _argc, char** _argv) {
    argc = _argc;
    argv = _argv;

    void* membuf = calloc(1, PHYSADDR_MASK);
    memcpy(membuf, MEMDATA, countof(MEMDATA));

    Emulator* emu = emu_create();

    mem_attachdev(emu->mem, &(BusDevice){.start_addr = 0x0,
                                         .end_addr = PHYSADDR_MASK,
                                         .read = testmem_read,
                                         .write = testmem_write,
                                         .free = free,
                                         .userdata = membuf});

    emu_reset(emu, true);

    while (true) {
        emu_step(emu);
    }

    emu_destroy(emu);

    return 0;
}

#endif
