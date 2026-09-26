.global _yield
.intel_syntax noprefix

.section .rodata
fmt:
    .string "yield: malloc failed with %d\n"

.section .text
    _yield:
        mov [rsp-0x98], rax
        mov [rsp-0x90], rcx
        mov [rsp-0x88], rdx
        mov [rsp-0x80], rbx
        mov [rsp-0x78], rsp
        mov [rsp-0x70], rbp
        mov [rsp-0x68], rdi
        mov [rsp-0x60], rsi
        mov [rsp-0x58], r8
        mov [rsp-0x50], r9
        mov [rsp-0x48], r10
        mov [rsp-0x40], r11
        mov [rsp-0x38], r12
        mov [rsp-0x30], r13
        mov [rsp-0x28], r14
        mov [rsp-0x20], r15

        // calculate stack_size
        mov rdi, rbp
        sub rdi, rsp
        mov [rsp-0x10], rdi

        // malloc in stack_buf
        sub rsp, 0x98
        call malloc
        test rax, rax
        jz .malloc_fail
        mov [rsp+0x90], rax

        // copy stack data into stack_buf
        mov rdi, rax
        mov rsi, rbp
        mov rcx, [rsp+0x88]
        rep movsb

        // save off rip
        mov rax, [rsp+0x98]
        mov [rsp+0x80], rax

        // return
        leave
        ret

        .malloc_fail:
        // call printf and then abort
        call __errno_location@PLT
        mov rsi, rax
        lea rdi, [rip + fmt]
        xor eax, eax
        call printf@PLT
        call abort@PLT
