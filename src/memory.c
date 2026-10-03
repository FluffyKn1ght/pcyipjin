#include "memory.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void mem_attachdev(Memory* mem, BusDevice* device) {
    mem->device_count++;
    memcpy(mem->devices + (mem->device_count - 1), device, sizeof(BusDevice));
}

u8 mem_read(Memory* mem, MemoryAccess access, u32 addr) {
    for (int i = 0; i < mem->device_count; i++) {
        if (mem->devices[i].start_addr <= addr && mem->devices[i].end_addr >= addr) {
            if (mem->devices[i].read) {
                return mem->devices[i].read(mem->devices[i].userdata, access, addr & PHYSADDR_MASK);
            }

            return MEM_UNDEFINED;
        }
    }

    return MEM_UNDEFINED;
}

void mem_write(Memory* mem, MemoryAccess access, u32 addr, u8 value) {
    for (int i = 0; i < mem->device_count; i++) {
        if (mem->devices[i].start_addr <= addr && mem->devices[i].end_addr >= addr) {
            if (mem->devices[i].write) {
                mem->devices[i].write(mem->devices[i].userdata, access, addr & PHYSADDR_MASK,
                                      value);
            }
            return;
        }
    }
}

void mem_free(Memory* mem) {
    for (int i = 0; i < mem->device_count; i++) {
        if (mem->devices->free) {
            mem->devices[i].free(mem->devices + i);
        }
    }

    free(mem);
}
