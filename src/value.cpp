#include "value.h"

// Chunk

int Chunk::addConstant(Value value) {
    for (int index = 0; index < (signed) constants.size(); index++) {
        Value &constant = constants[index];

        if (constant.type() == value.type()) {
            if (IS_NUMBER(value)) {
                if (AS_NUMBER(constant) == AS_NUMBER(value))
                    return index;
            } else if (IS_STRING(value)) {
                if (AS_STRING(constant)->str == AS_STRING(value)->str)
                    return index;
            }
        }
    }

    constants.push_back(value);
    return constants.size() - 1;
}

int Chunk::getLineNumber(int bytecodeIndex) {
    for (auto &[line, index] : lineNumbers) {
        if (bytecodeIndex >= index)
            return line;
    }
    return 0;
}

// Closure

ClosureObj::ClosureObj(FunctionValue function) : function(function) {
    upValues.reserve(function->upValueCount);
}
