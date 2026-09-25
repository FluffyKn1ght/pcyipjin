/**
 * @file pce/memory.h
 * @brief Emulates a 21-bit memory bus with a generalized mechanism for attaching devices
 */

#ifndef _PCEEMU_PCE_MEMORY_H
#define _PCEEMU_PCE_MEMORY_H

/**
 * @brief Represents different kinds of memory accesses
 */
typedef enum : u8 {
    MEMACCESS_CPU, /**< Memory accesses coming from the CPU */
} MemoryAccess;

/**
 * @brief Represents a device attached to a certain address range of a memory bus.
 */
typedef struct {
    u21 start_addr;                              /**< Start of address range */
    u21 end_addr;                                /**< End of address range */
    u8 (*read)(void*, MemoryAccess, u21);        /**< Read function */
    void (*write)(void*, MemoryAccess, u21, u8); /**< Write function */
    void*
        userdata; /**< Argument that will be passed into BusDevice::read() and BusDevice::write() */
} BusDevice;

/**
 * @brief Represents a memory bus with various devices connected to it.
 */
typedef struct {
    u16 device_count;
    BusDevice devices[];
} Memory;

/**
 * @brief Creates a new memory bus with no devices attached to it
 *
 * @note To free the memory bus created with this function, use arena_free()
 *
 * @returns The created memory bus
 */
Memory* mem_create();

/**
 * @brief Attaches a device to the memory bus
 *
 * @details This is not reversible, you can't detach a BusDevice once it's been connected (and
 * really, why would you need to?)
 *
 * @param mem_ptr A pointer to the Memory bus to attach the device to
 * @param device The BusDevice to attach to the bus
 */
void mem_attachdev(Memory** mem_ptr, BusDevice* device);

/**
 * @brief Reads a single byte from the memory bus
 *
 * @param mem The Memory bus to read from
 * @param access Which kind of memory access is occurring
 * @param addr The physical address to read from
 *
 * @return The read value
 */
u8 mem_read(Memory* mem, MemoryAccess access, u21 addr);

/**
 * @brief Writes a single byte to the memory bus
 *
 * @param mem The Memory bus to write to
 * @param access Which kind of memory access is occurring
 * @param addr The physical address to write to
 * @param value The value to write to the bus
 *
 * @return The read value
 */
void mem_write(Memory* mem, MemoryAccess access, u21 addr, u8 value);

#endif
