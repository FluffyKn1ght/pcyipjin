#include "utils/yield.h"

coroutine call(coroutine corout, coroutine (*func)(void*), void* arg) {
    if (corout == NULL) {
        return func();
    }

    asm("mov rdi, %0"
        "call %1" ::"r"(corout->rip),
        "r"(arg));
}
