#ifndef _YIELD_H
#define _YIELD_H

#include <string.h>

typedef struct {
    u64 rax, rcx, rdx, rbx;
    u64 rsp, rbp, rdi, rsi;
    u64 r8,  r9,  r10, r11;
    u64 r12, r13, r14, r15;
    u64 stack_size;
    void* stack_buf;
    void* rip;
} YieldBuf, *coroutine;

#define get_value(T, corout) ((T)(corout)->rdi)
#define stop() return NULL

#define yield(val) _yield((u64)(val))
void _yield(u64 value);

coroutine call(coroutine corout, coroutine (*func)(void*), void* arg);

#endif
