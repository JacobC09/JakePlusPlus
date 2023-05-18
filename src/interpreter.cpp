#include <cmath>
#include "interpreter.h"
#include "compiler.h"
#include "benchmark.h"
#include "print.h"

// Interpreter

Interpreter::Interpreter() {
    for (auto &[name, funcPtr] : nativeFunctions) {
        defineNative(name, funcPtr);
    }
}

InterpreterResult Interpreter::interpret(const char* source) {
    Parser parser = Parser(source);
    FunctionValue function = parser.compile();

    if (function == nullptr)
        return InterpreterResult::Error;

    resetStack();
    ClosureValue closure = std::make_shared<ClosureObj>(function);

    frames[frameCount++] = CallFrame(closure, stack);

    push(closure);

    InterpreterResult result = run();

    #ifdef DEBUGINFO
        printStack(stack, sp);
        printGlobals(globals);
    #endif

    return result;
}

Value Interpreter::pop() {
    sp--;
    return *sp;
}

Value Interpreter::peek(int offset) {
    return sp[-1 - offset];
}

UpValuePtrValue Interpreter::captureUpvalue(Value* local) {
    UpValuePtrValue prevUpValue = NULL;
    UpValuePtrValue upValue = openUpValues;

    while (upValue != NULL && upValue->location > local) {
        prevUpValue = upValue;
        upValue = upValue->next;
    }

    if (upValue != NULL && upValue->location == local) {
        return upValue;
    }

    UpValuePtrValue createdUpValue = std::make_shared<UpValueObj>(local);

    createdUpValue->next = upValue;

    if (prevUpValue == NULL) {
        openUpValues = createdUpValue;
    } else {
        prevUpValue->next = createdUpValue;
    }

    return createdUpValue;
}

void Interpreter::push(Value value) {
    *sp = value;
    sp++;
}

void Interpreter::runtimeError(std::string msg) {

    CallFrame* frame = &frames[frameCount - 1];
    int index = (int) (frame->ip - frame->closure->function->chunk.bytecode.data());
    
    printError(ExceptionType::RuntimeError, msg.c_str(), frame->closure->function->chunk.getLineNumber(index));

    for (int i = frameCount - 1; i >= 0; i--) {
        CallFrame* frame = &frames[i];
        FunctionValue function = frame->closure->function;
        
        int instruction = (frame->ip - function->chunk.bytecode.data() - 1);
        printf("[line %d] in ", function->chunk.getLineNumber(instruction));  // TODO: Make better errors
        
        if (!function->name.size()) {
            printf("script\n");
        } else {
            printf("%s()\n", function->name.c_str());
        }
    }

}

void Interpreter::resetStack() {
    sp = stack;
    frameCount = 0;
}

void Interpreter::closeUpValues(Value* last) {
    while (openUpValues != NULL && openUpValues->location >= last) {
        UpValuePtrValue upValue = openUpValues;
        upValue->closed = *upValue->location;
        upValue->location = &upValue->closed;
        openUpValues = upValue->next;
    }
}

void Interpreter::inhertClass(ClassValue subClass, ClassValue baseClass) {
    subClass->methods.insert(baseClass->methods.begin(), baseClass->methods.end());
}

void Interpreter::defineNative(std::string name, NativeFn function) {
    globals[name] = function;
}

void Interpreter::defineMethod(std::string name) {
    Value method = peek(0);
    ClassValue klass = AS_CLASS(peek(1));
    klass->methods[name] = method;
    pop();
}

bool Interpreter::callValue(Value value, u8 argc) {
    switch (value.type()) {
        case ValueType::Closure: {
            return callClosure(AS_CLOSURE(value), argc);
        }

        case ValueType::NativeFunc: {
            return callNativeFunction(AS_NATIVE_FUNCTION(value), argc);
        }

        case ValueType::Class: {
            ClassValue klass = AS_CLASS(value);
            sp[-argc - 1] = std::make_shared<InstanceObj>(klass);
            auto initializer = klass->methods.find(constructorName);
            if (initializer != klass->methods.end()) {
                callClosure(AS_CLOSURE(initializer->second), argc);
            } else if (argc != 0) {
                runtimeError(formatStr("Expected 0 arguments got %d", argc));
                return false;
            }
            break;
        }

        case ValueType::BoundMethod: {
            BoundMethodValue bound = AS_BOUND_METHOD(value);
            sp[-argc - 1] = bound->instance;
            return callClosure(bound->method, argc);
        }

        default:
            return false;
    }

    return true;
}

bool Interpreter::callClosure(ClosureValue closure, u8 argc) {
    if (frameCount + 1 > FRAMES_MAX) {
        runtimeError("Stack overflow");
        return false;
    }

    if (closure->function->argc != argc) {
        runtimeError(formatStr("Expcted %d arguments, got %d", closure->function->argc, argc));
        return false;
    }
    
    frameCount++;
    frames[frameCount] = CallFrame(closure, sp - argc - 1);

    return true;
}

bool Interpreter::callNativeFunction(NativeFuncValue nativeFunc, u8 argc) {
    Value result = (nativeFunc)(argc, sp - argc);
    
    if (IS_EXCEPTION(result)) {
        ExceptionValue exception = AS_EXCEPTION(result);
        runtimeError(exception->msg.c_str());
        return false;
    }
    
    sp -= argc + 1;
    push(result);
    return true;
}

bool Interpreter::invoke(std::string methodName, u8 argc) {
    Value value = AS_INSTANCE(peek(argc));

    if (!IS_INSTANCE(value)) {
        runtimeError("Only instances have methods");
        return false;
    }

    InstanceValue instance = AS_INSTANCE(value);

    auto field = instance->fields.find(methodName);

    if (field != instance->fields.end()) {
        sp[-argc - 1] = field->second;
        return callValue(value, argc);
    }

    return invokeFromClass(instance->klass, methodName, argc);
}

bool Interpreter::invokeFromClass(ClassValue klass, std::string methodName, u8 argc) {
    auto method = klass->methods.find(methodName);

    if (method == klass->methods.end()) {
        runtimeError(formatStr("Undefined property %s", methodName.c_str()));
    }

    return callClosure(AS_CLOSURE(method->second), argc);
}

bool Interpreter::bindMethod(ClassValue klass, std::string name) {
    auto value = klass->methods.find(name);

    if (value == klass->methods.end()) {
        runtimeError(formatStr("Instance of %s has no property %s", klass->name.c_str(), name.c_str()));
        return false;
    }

    BoundMethodValue bound = std::make_shared<BoundMethod>(AS_CLOSURE(value->second), peek(0));

    pop();
    push(bound);

    return true;
}

bool Interpreter::isFalsey(Value value) {
    return IS_NONE(value) || (IS_BOOLEAN(value) && !AS_BOOLEAN(value));
}

bool Interpreter::valuesEqual(Value valueA, Value valueB) {
    if (valueA.type() != valueB.type())
        return false;

    switch (valueA.type()) {
        case ValueType::Number: 
            return AS_NUMBER(valueA) == AS_NUMBER(valueB);
        case ValueType::Boolean:
            return AS_BOOLEAN(valueA) == AS_BOOLEAN(valueB);
        case ValueType::None:
            return true;

        default:
            return false;
    }

}

#define READ_BYTE() *frame->ip++
#define READ_CONSTANT() frame->closure->function->chunk.constants[READ_BYTE()]
#define READ_STRING() AS_STRING(READ_CONSTANT())
#define READ_SHORT() (frame->ip += 2, (u16) ((frame->ip[-1] << 8) | frame->ip[-2]))

InterpreterResult Interpreter::run() {
    CallFrame* frame = &frames[frameCount - 1];

    int pc = 0;

    for (;;) {
        u8 instruction = READ_BYTE();

        pc++;

        if (pc == 51) {
            print("made it to 51");
        }

        switch (instruction) {
            case OpPop: {
                pop();
                break;
            }

            case OpReturn: {
                Value result = pop();
                closeUpValues(frame->slots);
                frameCount--;
                
                if (frameCount == 0) {                    
                    pop();
                    return InterpreterResult::Success;
                }

                sp = frame->slots;
                push(result);
                frame = &frames[frameCount - 1];
                break;
            }
            
            case OpConstant: {
                push(READ_CONSTANT());
                break;
            }

            case OpTrue: {
                push(BOOLEAN_VAL(true));
                break;
            }

            case OpFalse: {
                push(BOOLEAN_VAL(false));
                break;
            }

            case OpNone: {
                push(NONE_VAL());
                break;
            }

            case OpAdd: {
                Value b = pop();
                Value a = pop();

                if (IS_NUMBER(a) && IS_NUMBER(b)) {
                    push(NUMBER_VAL(AS_NUMBER(a) + AS_NUMBER(b)));

                } else if (IS_STRING(a) && IS_STRING(b)) {
                    push(AS_STRING(a) + AS_STRING(b));
                } else {
                    runtimeError("Can only add numbers or strings");
                    return InterpreterResult::Error;
                }

                break;
            }

            case OpSubtract: {
                Value b = pop();
                Value a = pop();

                if (a.type() != ValueType::Number || b.type() != ValueType::Number) {
                    runtimeError("Can only subtract numbers");
                    return InterpreterResult::Error;
                }

                push(NUMBER_VAL(AS_NUMBER(a) - AS_NUMBER(b)));
                break;
            }

            case OpMultiply: {
                Value b = pop();
                Value a = pop();

                if (a.type() != ValueType::Number || b.type() != ValueType::Number) {
                    runtimeError("Can only multiply numbers");
                    return InterpreterResult::Error;
                }
                push(NUMBER_VAL(AS_NUMBER(a) * AS_NUMBER(b)));
                break;
            }

            case OpDivide: {
                Value b = pop();
                Value a = pop();

                if (a.type() != ValueType::Number || b.type() != ValueType::Number) {
                    runtimeError("Can only divide numbers");
                    return InterpreterResult::Error;
                }

                push(NUMBER_VAL(AS_NUMBER(a) / AS_NUMBER(b)));
                break;
            }

            case OpEqual: {
                Value b = pop();
                Value a = pop();

                if (a.type() != ValueType::Number || b.type() != ValueType::Number) {
                    runtimeError("Can only compair numbers");
                    return InterpreterResult::Error;
                }

                push(BOOLEAN_VAL(valuesEqual(a, b)));

                break;
            }

            case OpNotEqual: {
                Value b = pop();
                Value a = pop();

                if (a.type() != ValueType::Number || b.type() != ValueType::Number) {
                    runtimeError("Can only compair numbers");
                    return InterpreterResult::Error;
                }

                push(BOOLEAN_VAL(!valuesEqual(a, b)));

                break;
            }

            case OpGreater: {
                Value b = pop();
                Value a = pop();

                if (a.type() != ValueType::Number || b.type() != ValueType::Number) {
                    runtimeError("Can only compair numbers");
                    return InterpreterResult::Error;
                }

                push(BOOLEAN_VAL(AS_NUMBER(a) > AS_NUMBER(b)));

                break;
            }

            case OpLess: {
                Value b = pop();
                Value a = pop();
                
                if (a.type() != ValueType::Number || b.type() != ValueType::Number) {
                    runtimeError("Can only compair numbers");
                    return InterpreterResult::Error;
                }

                push(BOOLEAN_VAL(AS_NUMBER(a) < AS_NUMBER(b)));

                break;
            }

            case OpGreaterEqual: {
                Value b = pop();
                Value a = pop();

                if (a.type() != ValueType::Number || b.type() != ValueType::Number) {
                    runtimeError("Can only compair numbers");
                    return InterpreterResult::Error;
                }

                push(BOOLEAN_VAL(AS_NUMBER(a) >= AS_NUMBER(b)));

                break;
            }

            case OpLessEqual: {
                Value b = pop();
                Value a = pop();

                if (a.type() != ValueType::Number || b.type() != ValueType::Number) {
                    runtimeError("Can only compair numbers");
                    return InterpreterResult::Error;
                }

                push(BOOLEAN_VAL(AS_NUMBER(a) <= AS_NUMBER(b)));

                break;
            }

            case OpNegate: {
                Value a = pop();

                if (a.type() != ValueType::Number) {
                    runtimeError("Can only negate a number");
                    return InterpreterResult::Error;
                }

                push(NUMBER_VAL(-AS_NUMBER(a)));
                break;
            }

            case OpNot: {
                push(BOOLEAN_VAL(isFalsey(pop())));
                break;
            }

            case OpPrint: {
                printValue(pop());
                printf("\n");
                break;
            }

            case OpDefineGlobal: {
                StringValue str = READ_STRING();
                globals[str] = peek(0);
                pop();
                break;
            }

            case OpGetGlobal: {
                std::string name = READ_STRING();

                auto value = globals.find(name);

                if (value == globals.end()) {
                    runtimeError(formatStr("Undefined variable %s", name.c_str()));
                    return InterpreterResult::Error;
                }

                push(value->second);
                break;
            }

            case OpSetGlobal: {
                std::string name = READ_STRING();

                auto value = globals.find(name);

                if (value == globals.end()) {
                    runtimeError(formatStr("Undefined variable %s", name.c_str()));
                    return InterpreterResult::Error;
                }

                value->second = peek(0);
                break;
            }

            case OpGetLocal: {
                push(frame->slots[READ_BYTE()]);
                break;
            }

            case OpSetLocal: {
                frame->slots[READ_BYTE()] = peek(0);
                break;
            }

            case OpJump: {
                frame->ip += READ_SHORT();
                break;
            }

            case OpJumpBack: {
                frame->ip -= READ_SHORT();
                break;
            }

            case OpJumpIfTrue: {
                u16 distance = READ_SHORT();
                frame->ip += !isFalsey(peek(0)) * distance;
                break;
            }

            case OpJumpIfFalse: {
                u16 distance = READ_SHORT();
                frame->ip += isFalsey(peek(0)) * distance;
                break;
            }

            case OpCall: {
                u8 argc = READ_BYTE();
                Value value = peek(argc);

                if (!callValue(value, argc)) {
                    runtimeError("Invalid call target");
                    return InterpreterResult::Error;
                }
                
                frame = &frames[frameCount - 1];
                break;
            }
            
            case OpClosure: {
                FunctionValue function = AS_FUNCTION(READ_CONSTANT());
                ClosureValue closure = std::make_shared<ClosureObj>(function);
                push(closure);

                for (int i = 0; i < (signed) closure->function->upValueCount; i++) {
                    u8 isLocal = READ_BYTE();
                    u8 index = READ_BYTE();
                    if (isLocal) {
                        closure->upValues.push_back(captureUpvalue(frame->slots + index));
                    } else {
                        closure->upValues.push_back(frame->closure->upValues[index]);
                    }
                }

                break;
            }
            
            case OpGetUpValue: {
                u8 slot = READ_BYTE();
                push(*frame->closure->upValues[slot]->location);
                break;
            }

            case OpSetUpValue: {
                u8 slot = READ_BYTE();
                *frame->closure->upValues[slot]->location = peek(0);
                break;
            }

            case OpCloseUpValue: {
                closeUpValues(sp - 1);
                pop();
                break;
            }

            case OpClass: {
                push(std::make_shared<ClassObj>(READ_STRING()));
                break;
            }

            case OpGetProperty: {
                if (!IS_INSTANCE(peek(0))) {
                    runtimeError("Only instances have properties");
                    return InterpreterResult::Error;
                }
                
                InstanceValue instance = AS_INSTANCE(peek(0));
                std::string name = READ_STRING();

                auto field = instance->fields.find(name);

                if (field != instance->fields.end()) {
                    pop();
                    push(field->second);
                } else {
                    if (!bindMethod(instance->klass, name)) {
                        return InterpreterResult::Error;
                    }
                }


                break;
            }

            case OpSetProperty: {
                if (!IS_INSTANCE(peek(1))) {
                    runtimeError("Only instances have properties");
                    return InterpreterResult::Error;
                }

                InstanceValue instance = AS_INSTANCE(peek(1));

                instance->fields[READ_STRING()] = peek(0);
                Value value = pop();
                pop();
                push(value);
                break;
            }

            case OpMethod: {
                defineMethod(READ_STRING());
                break;
            }

            case OpInvoke: {
                std::string method = READ_STRING();
                int argc = READ_BYTE();

                if (!invoke(method, argc)) {
                    return InterpreterResult::Error;
                }

                frame = &frames[frameCount - 1];
                break;
            }

            case OpInherit: {
                Value baseClass = peek(1);

                if (!IS_CLASS(baseClass)) {
                    runtimeError("Can only inherit from a class");
                    return InterpreterResult::Error;
                }

                ClassValue subClass = AS_CLASS(peek(0));
                inhertClass(subClass, AS_CLASS(baseClass));
                
                pop();
                break;
            }

            case OpGetSuper: {
                std::string name = READ_STRING();
                ClassValue superSlass = AS_CLASS(pop());

                if (!bindMethod(superSlass, name)) {
                    return InterpreterResult::Error;
                }

                break;

            }

            default: {
                runtimeError(formatStr("Unknown Instruction (%d)", (int) instruction));
                return InterpreterResult::Error;
            }
        }
    }
}

#undef READ_BYTE
#undef READ_CONSTANT
#undef READ_STRING
#undef READ_SHORT