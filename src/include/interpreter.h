#pragma once
#include "common.h"
#include "scanner.h"
#include "jakelang.h"
#include "nativeFuncs.h"
#include "value.h"
#include "bytecode.h"

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

const std::string constructorName = "init";

enum class InterpreterResult {
    Success,
    Error
};

class CallFrame {
public:
    u8* ip;
    ClosureValue closure;
    Value* slots;

    CallFrame() = default;
    CallFrame(ClosureValue closure, Value* stack) : ip(closure->function->chunk.bytecode.data()), closure(closure), slots(stack) {};
};

class Interpreter {
public:
    Interpreter();
    InterpreterResult interpret(const char* source);

private:
    InterpreterResult run();
    void runtimeError(std::string msg);
    
    // Stack
    void push(Value value);
    void resetStack();
    Value pop();
    Value peek(int offset);
    
    // UpValues
    UpValuePtrValue captureUpvalue(Value* local);
    void closeUpValues(Value* last);
    
    // Inherit
    bool bindMethod(ClassValue klass, std::string name);
    void inhertClass(ClassValue subClass, ClassValue baseClass);

    // Define
    void defineNative(std::string name, NativeFn function);
    void defineMethod(std::string name);

    // Call
    bool callValue(Value value, u8 argc);
    bool callClosure(ClosureValue closure, u8 argc);
    bool callNativeFunction(NativeFuncValue nativeFunc, u8 argc);
    bool invoke(std::string methodName, u8 argc);
    bool invokeFromClass(ClassValue klass, std::string methodName, u8 argc);
    
    // Value
    bool isFalsey(Value value);
    bool valuesEqual(Value valueA, Value valueB);

    UpValuePtrValue openUpValues = NULL;
    std::map<std::string, Value> globals;

    int frameCount;
    CallFrame frames[FRAMES_MAX];

    Value* sp;
    Value stack[STACK_MAX];
};

// Debug
