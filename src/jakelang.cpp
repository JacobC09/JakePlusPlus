#include "jakelang.h"
#include "color.h"

// TODO: Add Stuff

void printError(ExceptionType type, std::string msg, int line, std::string value) {
    std::cout << color::red << color::bold;
    printf("Jake++ error on line %d:\n", line);

    printf("    %s: %s", exceptionNames[(int) type], msg.c_str());

    if (value.size())
        printf(" \'%s\'", value.c_str());
    
    std::cout << color::reset << std::endl;
}
