/**
 * @file utils/yield.h
 * @brief Implements Python's "yield" system, a return command that preserves function state
 * @author fgetcat
 */

#ifndef _YIELD_H
#define _YIELD_H

#include <string.h>

/**
 * @brief Holds the information about a yielded function state.
 *
 * @note Yielding functions are referred to as "coroutines"
 */
typedef struct {
    u64 rax, rcx, rdx, rbx;
    u64 rsp, rbp, rdi, rsi;
    u64 r8, r9, r10, r11;
    u64 r12, r13, r14, r15;
    u64 stack_size;
    void* stack_buf;
    void* rip;
} YieldBuf, *coroutine;

#define get_value(T, corout) ((T)(corout)->rdi)
#define stop() return NULL

#define yield(val) _yield((u64)(val))

/**
 * @brief Yields a coroutine, saving the function state/environment and returning the specified
 * value
 *
 * @note The yield() macro should be used instead for a smoother experience
 *
 * @param value The value to return
 */
void _yield(u64 value);

/**
 * @brief Calls a coroutine, continuing from where it left off
 *
 * @note free_coroutine() should be called after this returns NULL
 *
 * @param corout The YieldBuf object to resume from (NULL if this is the first call)
 * @param func The coroutine function to call
 * @param arg The argument to pass to the coroutine
 *
 * @return The saved function state (as a YieldBuf)
 * @retval NULL The coroutine has finished running
 */
coroutine call(coroutine corout, coroutine (*func)(void*), void* arg);

/**
 * @brief Frees a YieldBuf object and all of its internal buffers
 *
 * @note This should be called the coroutine has finished running (call() returns NULL)
 *
 * @param corout The YieldBuf object to free
 */
void free_coroutine(coroutine corout);

#endif
