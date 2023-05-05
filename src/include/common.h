#pragma once
#include <ostream>
#include <vector>
#include <string.h>
#include <map>
#include "debug.h"

#define DEBUGINFO

#define UINT8_COUNT 256
#define UINT8_MAX 255

#define UINT16_COUNT 65536
#define UINT16_MAX 65535

typedef float    f32;
typedef double   f64;
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;

std::string formatStr(const char* fmt, ...);
