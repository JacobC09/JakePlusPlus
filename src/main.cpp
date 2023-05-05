#include <fstream>
#include <string>
#include <sstream>
#include <chrono>
#include "benchmark.h"
#include "common.h"
#include "interpreter.h"
#include "color.h"

Interpreter interpreter;

#ifdef EMSCRIPTEN

#include "emscripten.h"
#include "emscripten/bind.h"

using namespace emscripten;

extern "C" {
    
    EMSCRIPTEN_KEEPALIVE
    void runString(const char* source) {
        interpreter.interpret(source);
    }

}

int main() {
    return 0;
}

#else

void repl() {
    print("Repl not defined yet");
    exit(1);
}

void runFile(const char* path) {
    std::fstream file;

    file.open(path);

    if (!file.is_open()) {
        print("[Error] Failed to open source file");
        return;
    }

    std::stringstream stream;
    stream << file.rdbuf();
    std::string source = stream.str();

    file.close();

    Timer<std::chrono::microseconds> clock;

    clock.tick();
    InterpreterResult status = interpreter.interpret(source.c_str());
    clock.tock();

    if (status == InterpreterResult::Error)
        std::cout << color::brightBlack << ">> Interpreter finished with error in " << clock.duration().count() / 100 << " milliseconds <<\n" << color::reset;
    else
        std::cout << color::brightBlack << ">> Interpreter finished in " << clock.duration().count() / 100 << " milliseconds <<\n" << color::reset;
}

int main(int argc, const char* argv[]) {
    if (argc == 1) {
        runFile("../code.jake");
    } else if (argc == 2) {
        runFile(argv[1]);
    } else {
        print("Usage: jake-lang [path]");
        exit(1);
    }
    
    return 0;
}

#endif