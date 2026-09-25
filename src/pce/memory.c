#include "pce/memory.h"
#include "utils/arena.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UNDEFINED 0xFF

Memory* mem_create() {
    Memory* mem = NULL;
    arena_alloc((void**)&mem, offsetof(Memory, devices));
    return mem;
}

void mem_attachdev(Memory** mem_ptr, BusDevice* device) {
    (*mem_ptr)->device_count++;
    arena_alloc((void**)mem_ptr,
                offsetof(Memory, devices) + sizeof(BusDevice) * (*mem_ptr)->device_count);
    memcpy((*mem_ptr)->devices + ((*mem_ptr)->device_count - 1), device, sizeof(BusDevice));
}

u8 mem_read(Memory* mem, MemoryAccess access, u32 addr) {
    for (int i = 0; i < mem->device_count; i++) {
        if (mem->devices[i].start_addr <= addr && mem->devices[i].end_addr >= addr) {
            return mem->devices[i].read(mem->devices[i].userdata, access, addr & PHYSADDR_MASK);
        }
    }

    return UNDEFINED;
}

void mem_write(Memory* mem, MemoryAccess access, u32 addr, u8 value) {
    for (int i = 0; i < mem->device_count; i++) {
        if (mem->devices[i].start_addr <= addr && mem->devices[i].end_addr >= addr) {
            mem->devices[i].write(mem->devices[i].userdata, access, addr & PHYSADDR_MASK, value);
            return;
        }
    }
}
