/**
 * @file pce/memory.h
 * @brief Emulates a 21-bit memory bus with a generalized mechanism for attaching devices
 */

#ifndef _PCYIPJIN_PCE_MEMORY_H
#define _PCYIPJIN_PCE_MEMORY_H

#define MEM_UNDEFINED 0xFF

#define BUS_DEVICE_COUNT 32

#define MEM_HUC6270_START 0x1FE000
#define MEM_HUC6270_END 0x1FE3FF

#define MEM_HUC6260_START 0x1FE400
#define MEM_HUC6260_END 0x1FE7FF

#define MEM_PSG_START 0x1FE800
#define MEM_PSG_END 0x1FEBFF

#define MEM_TIMER_START 0x1FEC00
#define MEM_TIMER_END 0x1FEFFF

#define MEM_IOPORT_START 0x1FF000
#define MEM_IOPORT_END 0x1FF3FF

#define MEM_CPU_INTCTL_START 0x1FF400
#define MEM_CPU_INTCTL_END 0x1FF7FF

#define MEM_RAM_START 0x1F0000
#define MEM_RAM_END 0x1FF7FF

/**
 * @brief Represents different kinds of memory accesses
 */
typedef enum : u8 {
    MEMACCESS_CPU, /**< Memory accesses coming from the CPU */
} MemoryAccess;

typedef u8 (*BusReadFunc)(void*, MemoryAccess, u32);
typedef void (*BusWriteFunc)(void*, MemoryAccess, u32, u8);
typedef void (*BusFreeFunc)(void*);

/**
 * @brief Represents a device attached to a certain address range of a memory bus.
 */
typedef struct {
    u32 start_addr;     /**< Start of address range */
    u32 end_addr;       /**< End of address range */
    BusReadFunc read;   /**< Read function */
    BusWriteFunc write; /**< Write function */
    BusFreeFunc free;   /**< Free function */
    void* userdata;     /**< Argument that will be passed into BusDevice functions */
} BusDevice;

/**
 * @brief Represents a memory bus with various devices connected to it.
 */
typedef struct {
    u16 device_count;
    BusDevice devices[BUS_DEVICE_COUNT];
} Memory;

/**
 * @brief Attaches a device to the memory bus
 *
 * @details This is not reversible, you can't detach a BusDevice once it's been connected (and
 * really, why would you need to?)
 *
 * @param mem The Memory bus to attach the device to
 * @param device The BusDevice to attach to the bus
 */
void mem_attachdev(Memory* mem, BusDevice* device);

/**
 * @brief Reads a single byte from the memory bus
 *
 * @param mem The Memory bus to read from
 * @param access Which kind of memory access is occurring
 * @param addr The physical address to read from
 *
 * @return The read value
 */
u8 mem_read(Memory* mem, MemoryAccess access, u32 addr);

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
void mem_write(Memory* mem, MemoryAccess access, u32 addr, u8 value);

/**
 * @brief Frees a Memory bus and all of its BusDevice objects
 *
 * @param mem The Memory bus to free
 */
void mem_free(Memory* mem);

#endif
