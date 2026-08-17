#include <llvm/Support/DynamicLibrary.h>
#include <llvm/ADT/StringRef.h>
#include <string>
#include <vector>
#include <cmath>

// -----------------------------------------------------------------------------
extern "C" {
    void printi(int val);
    void native_echo(long long val);
    float foo();
}
// -----------------------------------------------------------------------------
struct RuntimeFunction {
    std::string name;
    void* functionPointer;
};


void loadRuntimeLibrary() {
    std::vector<RuntimeFunction> registry = {
        { "printi",      (void*)&printi }
        ,{ "echo",        (void*)&native_echo }
        ,{ "print",       (void*)&std::printf }
        // FIXME return value .....
        ,{ "foo",         (void*)&foo }
        ,{ "sin",         (void*)&sinf }
        ,{ "cos",         (void*)&cosf }
        ,{ "sin_d",       (void*)&sin }
    };

    for (const auto& func : registry) {
        llvm::sys::DynamicLibrary::AddSymbol(func.name.c_str(), func.functionPointer);
    }
}
