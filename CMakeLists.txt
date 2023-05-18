cmake_minimum_required(VERSION 3.3)
project(JakeLang)

set(CMAKE_BUILD_TYPE Debug)

# ---------- Source Files ---------- #
add_executable(jake-lang 
    "src/main.cpp"
    "src/scanner.cpp"
    "src/jakelang.cpp"
    "src/interpreter.cpp"
    "src/compiler.cpp"
    "src/value.cpp"
    "src/common.cpp"
    "src/nativeFuncs.cpp"
)

# ---------- Linker Config ---------- #
target_include_directories(jake-lang PRIVATE "src/include/")

target_compile_options(jake-lang PRIVATE -Wall -std=c++20 -Wno-reorder)

target_precompile_headers(jake-lang PRIVATE "src/include/common.h")

# ---------- Emscripten ---------- #
if (EMSCRIPTEN)
    target_include_directories(jake-lang PRIVATE "C:/Program Files/emsdk/upstream/emscripten/cache/sysroot/include")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -sNO_EXIT_RUNTIME=1 -sEXPORTED_RUNTIME_METHODS=\"[ccall]\" -std=c++17")
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
endif()

# emcmake cmake .. -DPLATFORM=Web && cmake --build .
# --shell-file C:/Users/jakec/Documents/Coding/JakePlusPlus/.vscode/shell.html
# cls; cmake --build .; echo '======== Running ========'; ./jake-lang.exe; echo '======== Finished ========'; echo '';