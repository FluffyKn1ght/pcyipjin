#include "utils/yield.h"

__attribute__((naked)) void _yield(u64 value) { __asm__(
    "mov [rsp-0x98], rax\n"
    "mov [rsp-0x90], rcx\n"
    "mov [rsp-0x88], rdx\n"
    "mov [rsp-0x80], rbx\n"
    "mov [rsp-0x78], rsp\n"
    "mov [rsp-0x70], rbp\n"
    "mov [rsp-0x68], rdi\n"
    "mov [rsp-0x60], rsi\n"
    "mov [rsp-0x58], r8\n"
    "mov [rsp-0x50], r9\n"
    "mov [rsp-0x48], r10\n"
    "mov [rsp-0x40], r11\n"
    "mov [rsp-0x38], r12\n"
    "mov [rsp-0x30], r13\n"
    "mov [rsp-0x28], r14\n"
    "mov [rsp-0x20], r15\n"

    "mov rdi, rbp\n"
    "sub rdi, rsp\n"
    "mov [rsp-0x18], rdi\n"

    "sub rsp, 0x98\n"
    "call malloc\n"
    "mov [rsp+0x88], rax\n"

    "mov rdi, rax\n"
    "mov rsi, [rsp+0x28]\n"
    "mov rdx, [rsp+0x80]\n"
    "call memcpy\n"

    "mov rax, [rsp+0x98]\n"
    "mov [rsp+0x90], rax\n"

    "mov rdi, 0x98\n"
    "call malloc\n"
    "mov rdi, rax\n"
    "mov rsi, rsp\n"
    "mov rdx, 0x98\n"
    "call memcpy\n"

    "leave\n"
    "ret\n"
); }

coroutine call(coroutine corout, coroutine(*func)(void*), void* ctx) {
    if (!corout) return func(ctx);
    __asm__(
        "mov rdx, %0\n"

        "push rdx\n"
        "push [rdx+0x90]\n"

        "lea rax, [rip+1f]\n"
        "push rax\n"
        "push rbp\n"
        "mov rbp, rsp\n"
        "sub rsp, [rdx+0x80]\n"
        "and rsp, -16\n"

        "mov rdi, rsp\n"
        "mov rsi, [rdx+0x88]\n"
        "mov rcx, [rdx+0x80]\n"
        "rep movsb\n"

        "mov rax, [rdx+0x00]\n"
        "mov rcx, [rdx+0x08]\n"
        "mov rbx, [rdx+0x18]\n"
        "mov rdi, [rdx+0x30]\n"
        "mov rsi, [rdx+0x38]\n"
        "mov r8, [rdx+0x40]\n"
        "mov r9, [rdx+0x48]\n"
        "mov r10, [rdx+0x50]\n"
        "mov r11, [rdx+0x58]\n"
        "mov r12, [rdx+0x60]\n"
        "mov r13, [rdx+0x68]\n"
        "mov r14, [rdx+0x70]\n"
        "mov r15, [rdx+0x78]\n"
        "mov rdx, [rdx+0x10]\n"

        "jmp [rbp+0x10]\n"

        "1:\n"
        "mov rbx, rax\n"
        "mov rdi, [rbp+0x18]\n"
        "mov rdi, [rdi+0x88]\n"
        "call free\n"
        "mov rdi, [rbp+0x18]\n"
        "call free\n"
        "mov rax, rbx\n"
        "leave\n"
        "ret\n"
        :: "r"(corout)
    );
    __builtin_unreachable();
}
