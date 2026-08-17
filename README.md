# my_toy_compiler

## Modified by XXTH (2026-08-17)

- added CMakeList
- replaced a lot of code to get it run with llvm 22
- structured source into toy / scripts into script directory

## Build and run

    cmake -S . -B build
    cmake --build build
    ./parser scripts/example.toy


## debug

lldb ./parser
breakpoint set --file codegen.cpp --line 80
run example.txt


## Orig Code: 

Source code for "My Toy Compiler". Read about how he did on his blog:

http://gnuu.org/2009/09/18/writing-your-own-toy-compiler
