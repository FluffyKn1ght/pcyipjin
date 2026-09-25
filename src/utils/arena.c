#include "utils/arena.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define IDENT 0xBAAA00B3
#define IDENT_FREED 0xDEADBEEF
#define BLOCK_SIZE 4096

#define CHECK_IDENT()                                                                              \
    if (arena->ident != IDENT) {                                                                   \
        PRINTERR("invalid ident (bad alloc parameter?)");                                          \
        if (arena->ident == IDENT_FREED) {                                                         \
            PRINTERR("ident == IDENT_FREED (use-after-free?)");                                    \
        }                                                                                          \
        abort();                                                                                   \
        exit(1);                                                                                   \
    }

typedef struct {
    uint32_t ident;           /**< Magic value, used to detect bad function calls */
    uint64_t capacity_blocks; /**< Capacity of the arena, in blocks */
    uint64_t size;            /**< Size of the arena, in bytes */
} arena_t;

void arena_alloc(void** alloc_ptr, uintptr_t size) {
    arena_t* arena = NULL;

    if (!(*alloc_ptr)) {
        arena = calloc(sizeof(arena_t), 1);
        arena->ident = IDENT;
    } else {
        arena = (*alloc_ptr) - sizeof(arena_t);
        CHECK_IDENT();
    }

    uint64_t old_capacity = arena->capacity_blocks;
    uint64_t old_size = arena->size;

    arena->size += size;
    while (arena->size >= arena->capacity_blocks * BLOCK_SIZE) {
        arena->capacity_blocks++;
    }

    if (arena->capacity_blocks > old_capacity) {
        arena = realloc(arena, sizeof(arena_t) + arena->capacity_blocks * BLOCK_SIZE);
        (*alloc_ptr) = arena + 1;
    }
}

void arena_free(void* alloc) {
    arena_t* arena = alloc - sizeof(arena_t);
    CHECK_IDENT();

    // intentionally trash the ident
    arena->ident = IDENT_FREED;

    free(arena);
}

uint64_t arena_sizeof(void* alloc) {
    arena_t* arena = alloc - sizeof(arena_t);
    CHECK_IDENT();

    return arena->size;
}

uint64_t arena_memsizeof(void* alloc) {
    arena_t* arena = alloc - sizeof(arena_t);
    CHECK_IDENT();

    return arena->capacity_blocks * BLOCK_SIZE + sizeof(arena_t);
}
