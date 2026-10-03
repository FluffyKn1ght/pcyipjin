#include "memory.h"
#include <stdio.h>

u8 testmem_read(void* ptr, MemoryAccess _access, u32 addr) {
    printf(FILEPOS "read from $%06X\n", addr);
    return ((u8*)ptr)[addr];
}

void testmem_write(void* ptr, MemoryAccess _access, u32 addr, u8 value) {
    printf(FILEPOS "write to $%06X with $%02X\n", addr, value);
    ((u8*)ptr)[addr] = value;
}
