#ifndef _PCYIPJIN_TESTMEM
#define _PCYIPJIN_TESTMEM

#include "pce/memory.h"
#include <stdio.h>

u8 testmem_read(void* ptr, MemoryAccess _access, u32 addr);

void testmem_write(void* ptr, MemoryAccess _access, u32 addr, u8 value);

#endif
