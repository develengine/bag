#ifndef BAG_INCLUDE_H
#define BAG_INCLUDE_H

typedef unsigned long u64;
typedef signed long s64;
typedef unsigned int u32;
typedef signed int s32;
typedef unsigned short u16;
typedef signed short s16;
typedef unsigned char u8;
typedef signed char s8;

typedef float f32;
typedef double f64;

#ifndef countof
#define countof(x) (sizeof(x) / sizeof(x[0]))
#endif

#endif
