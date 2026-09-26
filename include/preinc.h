#include <stdint.h>

#define STR_(x) #x
#define STR(x) STR_(x)

#define PRINTERR(msg)                                                                              \
    printf("[" __FILE__ "::%s"                                                                     \
           "] " msg " (line " STR(__LINE__) ")",                                                   \
           __FUNCTION__)

#define PHYSADDR_MASK 0x1fffff

typedef uint8_t u8;
typedef int8_t s8;
typedef uint16_t u16;
typedef int16_t s16;
typedef uint32_t u32;
typedef int32_t s32;
typedef uint64_t u64;
typedef int64_t s64;
typedef float f32;
typedef double f64;
