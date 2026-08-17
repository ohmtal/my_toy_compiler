#include <llvm/Support/DynamicLibrary.h>
#include <llvm/ADT/StringRef.h>
#include <string>
#include <vector>

// -----------------------------------------------------------------------------
extern "C" {
    void printi(int val);
    void native_echo(long long val);
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
        ,{ "print",       (void*)&std::puts }
        // { "sin",       (void*)&std::sin },
        // { "cos",       (void*)&std::cos }
    };

    for (const auto& func : registry) {
        llvm::sys::DynamicLibrary::AddSymbol(func.name.c_str(), func.functionPointer);
    }
}
