#pragma once
#include <string>

namespace color {

#ifndef EMSCRIPTEN

    /* Decorators */
    const std::string reset = "\033[0m";
    const std::string bold = "\033[1m";
    const std::string underlined = "\033[4m";
    const std::string reversed = "\033[7m";

    /* Colors */
    const std::string black = "\033[30m";
    const std::string red = "\033[31m";
    const std::string green = "\033[32m";
    const std::string yellow = "\033[33m";
    const std::string blue = "\033[34m";
    const std::string purple = "\033[35m";
    const std::string aqua = "\033[36m";
    const std::string white = "\033[37m";
    
    /* Bright Colors */
    const std::string brightBlack = "\033[30;1m";
    const std::string brightRed = "\033[31;1m";
    const std::string brightGreen = "\033[32;1m";
    const std::string brightYellow = "\033[33;1m";
    const std::string brightBlue = "\033[34;1m";
    const std::string brightPurple = "\033[35;1m";
    const std::string brightAqua = "\033[36;1m";
    const std::string brightWhite = "\033[37;1m";
    
    /* Background Colors */
    const std::string bgBlack = "\033[40m";
    const std::string bgRed = "\033[41m";
    const std::string bgGreen = "\033[42m";
    const std::string bgYellow = "\033[43m";
    const std::string bgBlue = "\033[44m";
    const std::string bgPurple = "\033[45m";
    const std::string bgAqua = "\033[46m";
    const std::string bgWhite = "\033[47m";
    
    /* Bright Background Colors */
    const std::string bgBrightBlack = "\033[40;1m";
    const std::string bgBrightRed = "\033[41;1m";
    const std::string bgBrightGreen = "\033[42;1m";
    const std::string bgBrightYellow = "\033[43;1m";
    const std::string bgBrightBlue = "\033[44;1m";
    const std::string bgBrightPurple = "\033[45;1m";
    const std::string bgBrightAqua = "\033[46;1m";
    const std::string bgBrightWhite = "\033[47;1m";

#else

    /* Decorators */
    const std::string reset = "";
    const std::string bold = "";
    const std::string underlined = "";
    const std::string reversed = "";

    /* Colors */
    const std::string black = "";
    const std::string red = "";
    const std::string green = "";
    const std::string yellow = "";
    const std::string blue = "";
    const std::string purple = "";
    const std::string aqua = "";
    const std::string white = "";
    
    /* Bright Colors */
    const std::string brightBlack = "";
    const std::string brightRed = "";
    const std::string brightGreen = "";
    const std::string brightYellow = "";
    const std::string brightBlue = "";
    const std::string brightPurple = "";
    const std::string brightAqua = "";
    const std::string brightWhite = "";
    
    /* Background Colors */
    const std::string bgBlack = "";
    const std::string bgRed = "";
    const std::string bgGreen = "";
    const std::string bgYellow = "";
    const std::string bgBlue = "";
    const std::string bgPurple = "";
    const std::string bgAqua = "";
    const std::string bgWhite = "";
    
    /* Bright Background Colors */
    const std::string bgBrightBlack = "";
    const std::string bgBrightRed = "";
    const std::string bgBrightGreen = "";
    const std::string bgBrightYellow = "";
    const std::string bgBrightBlue = "";
    const std::string bgBrightPurple = "";
    const std::string bgBrightAqua = "";
    const std::string bgBrightWhite = "";

#endif

};
