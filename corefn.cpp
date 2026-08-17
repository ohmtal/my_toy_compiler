//XXTH completly rewritten

#include <iostream>
#include "codegen.h"
#include "node.h"


extern "C" void native_echo(long long value) {
    std::cout << value << std::endl;
}

void createEchoFunction(CodeGenContext& context)
{
    // 1. Define function signature: void echo(int64)
    std::vector<llvm::Type*> echo_arg_types;
    echo_arg_types.push_back(llvm::Type::getInt64Ty(MyContext));

    llvm::FunctionType* echo_type =
    llvm::FunctionType::get(llvm::Type::getVoidTy(MyContext), echo_arg_types, false);

    // 2. Create the 'echo' wrapper inside your language module
    llvm::Function *func = llvm::Function::Create(
        echo_type, llvm::Function::ExternalLinkage,
        "echo", context.module
    );

    llvm::BasicBlock *bblock = llvm::BasicBlock::Create(MyContext, "entry", func, 0);
    context.pushBlock(bblock);

    // 3. HARDCODE THE C++ ADDRESS INTO THE COMPILER (The Bulletproof Fix)
    // We fetch the literal runtime memory location of your compiled native_echo function
    uintptr_t nativeAddress = reinterpret_cast<uintptr_t>(&native_echo);

    // Convert that numeric address into an LLVM 64-bit integer constant
    llvm::Value* addrConstant = llvm::ConstantInt::get(
        llvm::Type::getInt64Ty(MyContext), nativeAddress
    );

    // Cast the raw 64-bit address into a valid LLVM generic function pointer (ptr)
    llvm::Value* functionPtr = llvm::CastInst::Create(
        llvm::Instruction::IntToPtr,
        addrConstant,
        llvm::PointerType::getUnqual(MyContext),
                                                      "funcPtrCast",
                                                      bblock
    );

    // 4. Collect the argument passed into 'echo' and pass it down
    std::vector<llvm::Value*> args;
    args.push_back(&*func->arg_begin()); // Pass the incoming 64-bit integer directly

    // 5. Execute the call using the explicit function pointer value and signature layout
    llvm::CallInst::Create(echo_type, functionPtr, llvm::ArrayRef<llvm::Value*>(args), "", bblock);
    llvm::ReturnInst::Create(MyContext, bblock);

    context.popBlock();
}

void createCoreFunctions(CodeGenContext& context){
       createEchoFunction(context);
}
