#pragma once

#define internal static
#define local_persist static
#define global static
#define ASSERT(Value) do { if (!(Value)) *(int *)0 = 0; } while (0)
#define ARRAY_COUNT(Array) (sizeof(Array)/sizeof((Array)[0]))

#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef bool b8;
typedef i32 b32;

typedef float f32;
typedef double f64;
