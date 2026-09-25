/**
 * @file utils/arena.h
 * @brief Arena memory allocator
 */

#ifndef _FLUFFY_ARENA_H
#define _FLUFFY_ARENA_H

#include <stdint.h>

/**
 * @brief Creates or expands an arena allocation.
 *
 * @details The new allocation address will be written to alloc_ptr, and it may change after calling
 * this function (for example, due to an internal realloc() call).
 * Technically, the arenas use
 *
 * @warning The address obtained from this function is NOT free()able! Use arena_free() instead.
 *
 * @param alloc_ptr A pointer to the variable holding the allocation address
 * @param size How much memory to add to the allocation
 */
void arena_alloc(void** alloc_ptr, uintptr_t size);

/**
 * @brief Frees an arena allocation.
 *
 * @param alloc The allocation to free
 */
void arena_free(void* alloc);

/**
 * @brief Gets the current allowed usable size of an arena allocation, in bytes.
 *
 * @warning This is not a valid way of measuring arena memory usage and is meant for stuff like
 * getting the size of a buffer represented by an arena allocation. To get the actual memory
 * footprint of an arena allocation, use arena_memsizeof() instead.
 *
 * @param alloc The arena allocation to query.
 *
 * @return The size of the arena.
 */
uint64_t arena_sizeof(void* alloc);

/**
 * @brief Gets the current memory footprint of an arena allocation, in bytes.
 *
 * @details This might be bigger than arena_sizeof() because the arena allocator allocates blocks
 * and not individual bytes. This includes EVERYTHING, including unused portions of allocated blocks
 * in the arena and the arena header.
 *
 * @warning This is not a valid way of checking how much data you can/should write into an arena
 * allocation. Using the function for that will result in big, BIG issues. To check how much data
 * you can put into an arena, use arena_sizeof() instead.
 *
 * @param alloc The arena allocation to query.
 *
 * @return The full memory size of the arena, in bytes.
 */
uint64_t arena_memsizeof(void* alloc);

#endif
