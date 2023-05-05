#pragma once
#include "common.h"

enum ExceptionType {
    SyntaxError,
    RuntimeError
};

inline const char* exceptionNames[] = {"SyntaxError", "RuntimeError"};

void printError(ExceptionType type, std::string msg, int line=0, std::string value="");

#include <stdio.h>