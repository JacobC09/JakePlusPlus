#include <stdarg.h>
#include "common.h"

std::string formatStr(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int size = vsnprintf(nullptr, 0, fmt, args);
    va_end(args);

    char buffer[size + 1];
    va_start(args, fmt);
    vsnprintf(buffer, size + 1, fmt, args);
    va_end(args);

    return std::string(buffer);
}