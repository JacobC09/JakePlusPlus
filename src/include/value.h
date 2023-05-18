#pragma once
#include <map>
#include <variant>
#include <memory>
#include "common.h"
#include "jakelang.h"

class Value;

typedef Value (*NativeFn)(int argc, Value argv[]);

class StringObj;
class FunctionObj;
class UpValueObj;
class ClosureObj;
class NativeFuncObj;
class ExceptionObj;
class ClassObj;
class InstanceObj;
class BoundMethod;

class NativeFuncObj {
public:
    NativeFn funcPtr;

    NativeFuncObj() = default;
    NativeFuncObj(NativeFn funcPtr) : funcPtr(funcPtr) {};
};


using NoneValue = std::monostate;
using NumberValue = double;
using BooleanValue = bool;
using StringValue = std::string;
using FunctionValue = std::shared_ptr<FunctionObj>;
using UpValuePtrValue = std::shared_ptr<UpValueObj>;
using ClosureValue = std::shared_ptr<ClosureObj>;
using NativeFuncValue = NativeFuncObj;
using ExceptionValue = std::shared_ptr<ExceptionObj>;
using ClassValue = std::shared_ptr<ClassObj>;
using InstanceValue = std::shared_ptr<InstanceObj>;
using BoundMethodValue = std::shared_ptr<BoundMethod>;

enum class ValueType {
    None,
    Number,
    Boolean,
    String,
    Function,
    UpValuePtr,
    Closure,
    NativeFunc,
    Exception,
    Class,
    Instance,
    BoundMethod
};

using ValueVariant = std::variant<NoneValue, NumberValue, BooleanValue, StringValue, FunctionValue, UpValuePtrValue, ClosureValue, NativeFuncValue, ExceptionValue, ClassValue, InstanceValue, BoundMethodValue>;

class Value : public ValueVariant {
public:
    using ValueVariant::variant;

    ValueType type() {
        return (ValueType) index();
    }

    template <typename T>
    T as() {
        return std::get<T>(*this);
    }
};

class Chunk {
public:
    std::vector<u8> bytecode;
    std::vector<Value> constants;
    std::map<int, int> lineNumbers;

    int addConstant(Value value);
    int getLineNumber(int bytecodeIndex);
};

class FunctionObj {
public:
    int argc = 0;
    int upValueCount = 0;
    std::string name;
    Chunk chunk;

    FunctionObj() : chunk(Chunk()) {};
};

class UpValueObj {
public:
    Value* location;
    UpValuePtrValue next = NULL;
    Value closed;

    UpValueObj() = default;
    UpValueObj(Value* location) : location(location) {};
};

class ClosureObj {
public:
    FunctionValue function;
    std::vector<UpValuePtrValue> upValues;

    ClosureObj() = default;
    ClosureObj(FunctionValue function);
};

class ExceptionObj {
public:
    std::string msg;
    ExceptionType type;

    ExceptionObj() = default;
    ExceptionObj(std::string msg, ExceptionType type) : msg(msg), type(type) {};
};

class ClassObj {
public:
    std::string name;
    std::map<std::string, Value> methods;

    ClassObj() = default;
    ClassObj(std::string name) : name(name) {};
};

class InstanceObj {
public:
    ClassValue klass;
    std::map<std::string, Value> fields;

    InstanceObj() = default;
    InstanceObj(ClassValue klass) : klass(klass) {};
};

class BoundMethod {
public:
    ClosureValue method;
    Value instance;

    BoundMethod() = default;
    BoundMethod(ClosureValue method, Value receiver) : method(method), instance(receiver) {};
};

#define NUMBER_VAL(value) (value)
#define BOOLEAN_VAL(value) (value)
#define NONE_VAL() (std::monostate{})

#define IS_NUMBER(value)  ((value).type() == ValueType::Number)
#define IS_BOOLEAN(value)  ((value).type() == ValueType::Boolean)
#define IS_NONE(value) ((value).type() == ValueType::None)

#define AS_NUMBER(value) (std::get<NumberValue>(value))
#define AS_BOOLEAN(value) (std::get<BooleanValue>(value))

#define IS_STRING(value) ((value).type() == ValueType::String)
#define IS_FUNCTION(value) ((value).type() == ValueType::Function)
#define IS_CLOSURE(value) ((value).type() == ValueType::Closure)
#define IS_NATIVE_FUNCTION(value) ((value).type() == ValueType::NativeFunc)
#define IS_EXCEPTION(value) ((value).type() == ValueType::Exception)
#define IS_UPVALUE(value) ((value).type() == ValueType::UpValue)
#define IS_CLASS(value) ((value).type() == ValueType::Class)
#define IS_INSTANCE(value) ((value).type() == ValueType::Instance)
#define IS_BOUND_METHOD(value) ((value).type() == ValueType::Instance)

#define AS_STRING(obj) (std::get<StringValue>(obj))
#define AS_FUNCTION(obj) (std::get<FunctionValue>(obj))
#define AS_CLOSURE(obj) (std::get<ClosureValue>(obj))
#define AS_NATIVE_FUNCTION(obj) (std::get<NativeFuncValue>(obj))
#define AS_EXCEPTION(obj) (std::get<ExceptionValue>(obj))
#define AS_UPVALUE(obj) (std::get<UpValuePtrValue>(obj))
#define AS_CLASS(obj) (std::get<ClassValue>(obj))
#define AS_INSTANCE(obj) (std::get<InstanceValue>(obj))
#define AS_BOUND_METHOD(obj) (std::get<BoundMethodValue>(obj))
