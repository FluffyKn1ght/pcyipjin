#include "pce/memory.h"
#include <stdio.h>

u8 testmem_read(void* ptr, MemoryAccess _access, u32 addr) {
    printf(PRINT_FILEPOS "read from %x\n", addr);
    return ((u8*)ptr)[addr];
}

void testmem_write(void* ptr, MemoryAccess _access, u32 addr, u8 value) {
    printf(PRINT_FILEPOS "write to %x with %x\n", addr, value);
    ((u8*)ptr)[addr] = value;
}
