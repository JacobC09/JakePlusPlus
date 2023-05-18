#pragma once
#include "common.h"
#include "value.h"

namespace BuiltIn {

    Value nativePow(int argc, Value argv[]);
    Value nativeSqrt(int argc, Value argv[]);
    Value nativeClock(int argc, Value argv[]);

}

const std::map<std::string, NativeFn> nativeFunctions = {
    {"pow", &BuiltIn::nativePow},
    {"sqrt", &BuiltIn::nativeSqrt},
    {"clock", &BuiltIn::nativeClock},
};
