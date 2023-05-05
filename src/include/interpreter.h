#pragma once
#include <map>
#include "common.h"
#include "scanner.h"
#include "jakelang.h"
#include "value.h"

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

const std::string constructorName = "init";

enum Bytecode : u8 {
    OpPop,
    OpReturn,
    OpConstant,
    OpTrue,
    OpFalse,
    OpNone,
    OpAdd,
    OpSubtract,
    OpMultiply,
    OpDivide,
    OpEqual,
    OpNotEqual,
    OpGreater,
    OpLess,
    OpGreaterEqual,
    OpLessEqual,
    OpNot,
    OpNegate,
    OpPrint,
    OpDefineGlobal,
    OpGetGlobal,
    OpSetGlobal,
    OpGetLocal,
    OpSetLocal,
    OpGetUpValue,
    OpSetUpValue,
    OpCloseUpValue,
    OpJump,
    OpJumpBack,
    OpJumpIfTrue,
    OpJumpIfFalse,
    OpCall,
    OpClosure,
    OpClass,
    OpGetProperty,
    OpSetProperty,
    OpMethod,
    OpInvoke,
    OpInherit,
    OpGetSuper
};

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
    Value pop();
    Value peek(int offset);
    UpValuePtrValue captureUpvalue(Value* local);

    void push(Value value);
    void runtimeError(const char* format, ...);
    void resetStack();
    void closeUpValues(Value* last);
    void inhertClass(ClassValue subClass, ClassValue baseClass);
    void defineNative(std::string name, NativeFn function);
    void defineMethod(std::string name);

    bool callValue(Value value, u8 argc);
    bool callClosure(ClosureValue closure, u8 argc);
    bool callNativeFunction(NativeFuncValue nativeFunc, u8 argc);
    bool invoke(std::string methodName, u8 argc);
    bool invokeFromClass(ClassValue klass, std::string methodName, u8 argc);
    bool bindMethod(ClassValue klass, std::string name);
    bool isFalsey(Value value);
    bool valuesEqual(Value valueA, Value valueB);

    UpValuePtrValue openUpValues = NULL;
    std::map<std::string, Value> globals;

    int frameCount;
    CallFrame frames[FRAMES_MAX];

    Value* sp;
    Value stack[STACK_MAX];
};

// Native Functions

Value internalPow(int argc, Value argv[]);

inline std::map<std::string, NativeFn> nativeFunctions = {
    {"pow", &internalPow}
};

// Debug

inline void printValue(Value value) {
    switch (value.type()) {
        case ValueType::Number:
            printf("%g", AS_NUMBER(value));
            break;

        case ValueType::Boolean:
            if (AS_BOOLEAN(value))
                printf("true");
            else
                printf("false");
            break;

        case ValueType::None:
            printf("None");
            break;

        case ValueType::String:
            printf("%s", AS_STRING(value)->str.c_str());
            break;
        
        case ValueType::Function:
            if (!AS_FUNCTION(value)->name.size()) {
                printf("<script>");
            } else {
                printf("<fn %s>", AS_FUNCTION(value)->name.c_str());
            }
            break;

        case ValueType::Closure:
            if (!AS_CLOSURE(value)->function->name.size()) {
                printf("<script>");
            } else {
                printf("<fn %s>", AS_CLOSURE(value)->function->name.c_str());
            }
            break;

        case ValueType::NativeFunc:
            printf("<native fn>");
            break;

        case ValueType::UpValuePtr:
            printf("<upvalue>");
            break;

        case ValueType::Class:
            printf("<class %s>", AS_CLASS(value)->name.c_str());
            break;

        case ValueType::Instance:
            printf("<%s instance>", AS_INSTANCE(value)->klass->name.c_str());
            break;

        case ValueType::BoundMethod:
            if (!AS_BOUND_METHOD(value)->method->function->name.size()) {
                printf("<bound script>");
            } else {
                printf("<bound fn %s>", AS_BOUND_METHOD(value)->method->function->name.c_str());
            }

        default:
            break;
    }
}

inline int simpleInstruction(const char* name, int index) {
    printf("%s\n", name);
    return index + 1;
}

inline int constantInstruction(const char* name, Chunk* chunk, int index) {
    u8 constant = chunk->bytecode[index + 1];
    printf("%-16s %d '", name, constant);

    printValue(chunk->constants[constant]);
    printf("'\n");

    return index + 2;
}

inline int byteInstruction(const char* name, Chunk* chunk, int index) {
    printf("%-16s %4d\n", name, chunk->bytecode[index + 1]);
    return index + 2;
}

inline int jumpInstruction(const char* name, Chunk* chunk, int factor, int index) {
    int distance = ((chunk->bytecode[index + 2] << 8) | chunk->bytecode[index + 1]);
    printf("%-16s %d -> %d\n", name, index, index + distance * factor + 3);
    return index + 3;
}

inline int invokeInstruction(const char* name, Chunk* chunk, int index) {
    u8 constant = chunk->bytecode[index + 1];
    u8 argCount = chunk->bytecode[index + 2];
    printf("%-16s (%d args) %4d '", name, argCount, constant);
    printValue(chunk->constants[constant]);
    printf("'\n");
    return index + 3;
}

inline int disassembleInstruction(Chunk* chunk, int index) {
    printf("%04d ", index);
    
    switch (chunk->bytecode[index]) {
        case OpPop:
            return simpleInstruction("Pop", index);

        case OpReturn:
            return simpleInstruction("Return", index);

        case OpConstant:
            return constantInstruction("Constant", chunk, index);

        case OpTrue:
            return simpleInstruction("True", index);
        
        case OpFalse:
            return simpleInstruction("False", index);
        
        case OpNone:
            return simpleInstruction("None", index);

        case OpAdd:
            return simpleInstruction("Add", index);

        case OpSubtract:
            return simpleInstruction("Subtract", index);

        case OpMultiply:
            return simpleInstruction("Multiply", index);

        case OpDivide:
            return simpleInstruction("Divide", index);
        
        case OpEqual:
            return simpleInstruction("Equal", index);

        case OpNotEqual:
            return simpleInstruction("NotEqual", index);

        case OpGreater:
            return simpleInstruction("Greater", index);

        case OpLess:
            return simpleInstruction("Less", index);

        case OpGreaterEqual:
            return simpleInstruction("GreaterEqual", index);

        case OpLessEqual:
            return simpleInstruction("LessEqual", index);

        case OpNegate:
            return simpleInstruction("Negate", index);

        case OpNot:
            return simpleInstruction("Not", index);

        case OpPrint:
            return simpleInstruction("Print", index);
        
        case OpDefineGlobal:
            return constantInstruction("DefineGlobal", chunk, index);
        
        case OpGetGlobal:
            return constantInstruction("GetGlobal", chunk, index);
        
        case OpSetGlobal:
            return constantInstruction("SetGlobal", chunk, index);
        
        case OpGetLocal:
            return byteInstruction("GetLocal", chunk, index);
        
        case OpSetLocal:
            return byteInstruction("SetLocal", chunk, index);
        
        case OpGetUpValue:
            return byteInstruction("GetUpValue", chunk, index);
        
        case OpSetUpValue:
            return byteInstruction("SetUpValue", chunk, index);

        case OpJump:
            return jumpInstruction("Jump", chunk, 1, index);

        case OpJumpBack:
            return jumpInstruction("JumpBack", chunk, -1, index);

        case OpJumpIfTrue:
            return jumpInstruction("JumpIfTrue", chunk, 1, index);

        case OpJumpIfFalse:
            return jumpInstruction("JumpIfFalse", chunk, 1, index);
        
        case OpCall:
            return byteInstruction("Call", chunk, index);

        case OpCloseUpValue:
            return byteInstruction("CloseUpValue", chunk, index);

        case OpClosure: {
            index++;
            u8 constant = chunk->bytecode[index++];
            printf("%-16s %4d ", "Closure", constant);
            printValue(chunk->constants[constant]);
            printf("\n");

            FunctionValue function = AS_FUNCTION(chunk->constants[constant]);
            for (int j = 0; j < function->upValueCount; j++) {
                int isLocal = chunk->bytecode[index++];
                int valueIndex = chunk->bytecode[index++];
                printf("%04d   |                   %s %d\n", index - 2, isLocal ? "local" : "upvalue", valueIndex);
            }

            return index;
        }

        case OpClass:
            return constantInstruction("Class", chunk, index);

        case OpGetProperty:
            return constantInstruction("GetProperty", chunk, index);

        case OpSetProperty:
            return constantInstruction("SetProperty", chunk, index);

        case OpMethod:
            return constantInstruction("Method", chunk, index);
        
        case OpInvoke:
            return invokeInstruction("Invoke", chunk, index);

        case OpInherit:
            return simpleInstruction("Inherit", index);

        case OpGetSuper:
            return constantInstruction("GetSuper", chunk, index);

        default:
            printf("Unknown Instruction\n");
            return index + 1;
    }
}

inline void printConstants(Chunk* chunk) {
    printf(">== Constants ==<");
    for (int index = 0; index < (signed) chunk->constants.size(); index++) {
        printf("\n[%d] ", index);
        printValue(chunk->constants[index]);
    }
    printf("\n>=============<\n");
}

inline void disassembleChunk(Chunk* chunk, std::string name="") {
    if (name == "")
        name = "Disassembled Chunk";

    printf(">== %s ==<\n", name.c_str());

    for (int index = 0; index < (signed) chunk->bytecode.size();) {
        index = disassembleInstruction(chunk, index);
    }

    printf(">===%s===<\n", std::string(name.size(), '=').c_str());
}

inline void printStack(Value stack[], Value* sp) {
    printf(">== Stack ==<");
    
    int stackSlots = (int) (sp - stack);
    for (int index = 0; index < stackSlots; index++) {
      printf("\n[%d] ", index);
      printValue(stack[index]);
    }
    printf("\n>=========<\n");
}

inline void printGlobals(std::map<std::string, Value> &globals) {
    printf(">== Globals ==<");

    for (auto &[key, value] : globals) {
        printf("\n%s: ", key.c_str());
        printValue(value);
    }

    printf("\n>=============<\n");
}
