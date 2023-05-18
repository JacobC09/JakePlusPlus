#include <cmath>
#include <ctime>
#include "common.h"
#include "value.h"
#include "nativeFuncs.h"

#define NATIVE_RUNTIME_ERROR(msg) std::make_shared<ExceptionObj>(msg, ExceptionType::RuntimeError)
#define ASSERT_ARG_COUNT(count) if (argc != count) return NATIVE_RUNTIME_ERROR(formatStr("Expected %d arguments, got %d", count, argc))
#define ASSERT_TYPE(argIndex, TYPE_MACRO, msg) if (!TYPE_MACRO(argv[argIndex])) return NATIVE_RUNTIME_ERROR(msg)

Value BuiltIn::nativePow(int argc, Value argv[]) {
    ASSERT_ARG_COUNT(2);
    
    ASSERT_TYPE(0, IS_NUMBER, "Expected argument 1 as number");
    ASSERT_TYPE(1, IS_NUMBER, "Expected argument 2 as number");

    NumberValue result = pow(AS_NUMBER(argv[0]), AS_NUMBER(argv[1]));

    return result;
}

Value BuiltIn::nativeSqrt(int argc, Value argv[]) {
    ASSERT_ARG_COUNT(1);
    
    ASSERT_TYPE(0, IS_NUMBER, "Expected argument 1 as number");

    NumberValue result = sqrt(AS_NUMBER(argv[0]));

    return result;
}

Value BuiltIn::nativeClock(int argc, Value argv[]) {
    ASSERT_ARG_COUNT(0);

    return NUMBER_VAL((double) clock());
}