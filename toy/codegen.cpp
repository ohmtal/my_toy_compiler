#include "node.h"
#include "codegen.h"
#include "parser.hpp"

#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "llvm/ExecutionEngine/RuntimeDyld.h"

#include <string>
#include <llvm/IR/Value.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Instructions.h>

using namespace std;

/* Compile the AST into a module */
void CodeGenContext::generateCode(NBlock& root)
{
	std::cout << "Generating code...\n";
	
	/* Create the top level interpreter function to call as entry */
	vector<Type*> argTypes;
	// XXTH orig FunctionType *ftype = FunctionType::get(Type::getVoidTy(MyContext), makeArrayRef(argTypes), false);
	FunctionType *ftype = FunctionType::get(Type::getVoidTy(MyContext), std::vector<Type*>(argTypes), false);
	mainFunction = Function::Create(ftype, GlobalValue::InternalLinkage, "main", module);
	BasicBlock *bblock = BasicBlock::Create(MyContext, "entry", mainFunction, 0);
	
	/* Push a new variable/block context */
	pushBlock(bblock);
	root.codeGen(*this); /* emit bytecode for the toplevel block */
	ReturnInst::Create(MyContext, bblock);
	popBlock();
	
	/* Print the bytecode in a human-readable format 
	   to see if our program compiled properly
	 */
	std::cout << "Code is generated.\n";
	// module->dump();

	legacy::PassManager pm;
	// TODO:
	pm.add(createPrintModulePass(outs()));
	pm.run(*module);
}



class ToyRuntimeResolver : public llvm::SectionMemoryManager {
	std::map<std::string, void*> &symbols;
public:
	ToyRuntimeResolver(std::map<std::string, void*> &symbolsMap) : symbols(symbolsMap) {}

	llvm::JITSymbol findSymbol(const std::string &Name) override {
		std::string cleanName = Name;
		if (!cleanName.empty() && cleanName[0] == '_') {
			cleanName = cleanName.substr(1);
		}

		if (symbols.count(cleanName)) {
			uintptr_t addr = reinterpret_cast<uintptr_t>(symbols[cleanName]);
			return llvm::JITSymbol(addr, llvm::JITSymbolFlags::Exported | llvm::JITSymbolFlags::Absolute);
		}

		return llvm::SectionMemoryManager::findSymbol(Name);
	}

	llvm::JITSymbol findSymbolInLogicalDylib(const std::string &Name) override {
		std::string cleanName = Name;
		if (!cleanName.empty() && cleanName[0] == '_') {
			cleanName = cleanName.substr(1);
		}

		if (symbols.count(cleanName)) {
			uintptr_t addr = reinterpret_cast<uintptr_t>(symbols[cleanName]);
			return llvm::JITSymbol(addr, llvm::JITSymbolFlags::Exported | llvm::JITSymbolFlags::Absolute);
		}

		return llvm::SectionMemoryManager::findSymbolInLogicalDylib(Name);
	}
};
// -----------------------------------------------------------------------------
GenericValue CodeGenContext::runCode() {
	std::cout << "Running code...\n";

	std::unique_ptr<llvm::Module> modPtr(module);
	module = nullptr;

	llvm::EngineBuilder builder(std::move(modPtr));
	builder.setEngineKind(llvm::EngineKind::JIT);

	auto memMgr = std::make_unique<llvm::SectionMemoryManager>();
	builder.setMCJITMemoryManager(std::move(memMgr));

	std::unique_ptr<llvm::ExecutionEngine> ee(builder.create());
	if (!ee) {
		std::cerr << "CRITICAL ERROR: JIT Execution Engine could not be created!\n";
		exit(1);
	}

	std::cout << "Finalizing object...\n";
	ee->finalizeObject();

	llvm::Function *mainFunction = ee->FindFunctionNamed("main");
	if (!mainFunction) {
		std::cerr << "CRITICAL ERROR: 'main' function not found!\n";
		exit(1);
	}

	std::cout << "Invoking main function...\n";
	GenericValue v = ee->runFunction(mainFunction, llvm::ArrayRef<GenericValue>());
	std::cout << "Code was run.\n";

	return v;
}
// -----------------------------------------------------------------------------
/* Returns an LLVM type based on the identifier */
static Type *typeOf(const NIdentifier& type)
{
	if (type.name.compare("int") == 0) {
		return Type::getInt64Ty(MyContext);
	}
	else if (type.name.compare("double") == 0) {
		return Type::getDoubleTy(MyContext);
	}
	else if (type.name == "string") {
		return llvm::PointerType::getUnqual(MyContext);
	}
	return Type::getVoidTy(MyContext);
}

/* -- Code Generation -- */

Value* NInteger::codeGen(CodeGenContext& context)
{
	std::cout << "Creating integer: " << value << endl;
	return ConstantInt::get(Type::getInt64Ty(MyContext), value, true);
}

Value* NDouble::codeGen(CodeGenContext& context)
{
	std::cout << "Creating double: " << value << endl;
	return ConstantFP::get(Type::getDoubleTy(MyContext), value);
}


Value* NIdentifier::codeGen(CodeGenContext& context)
{
	std::cout << "Creating identifier reference: " << name << endl;
	if (context.locals().find(name) == context.locals().end()) {
		std::cerr << "undeclared variable " << name << endl;
		// bad idea return NULL;
		std::exit(1);
	}

	// return nullptr;
	return new LoadInst(context.locals()[name]->getType(),context.locals()[name], name, false, context.currentBlock());
}

// -----------------------------------------------------------------------------
llvm::Value* NMethodCall::codeGen(CodeGenContext& context)
{
	llvm::Function* function = context.module->getFunction(id.name.c_str());

	if (!function) {
		std::cerr << "Semantics Error: Unknown function " << id.name << "\n";
		// bad idea return nullptr;
		std::exit(1);
	}

	std::vector<llvm::Value*> args;
	for (auto argExpr : arguments) {
		args.push_back(argExpr->codeGen(context));
	}

	return llvm::CallInst::Create( function, llvm::ArrayRef<llvm::Value*>(args), "", context.currentBlock() );
}
// -----------------------------------------------------------------------------

Value* NBinaryOperator::codeGen(CodeGenContext& context)
{

	std::cout << "Creating binary operation " << op << endl;
	Instruction::BinaryOps instr;
	switch (op) {
		case TPLUS: 	instr = Instruction::Add; goto math;
		case TMINUS: 	instr = Instruction::Sub; goto math;
		case TMUL: 		instr = Instruction::Mul; goto math;
		case TDIV: 		instr = Instruction::SDiv; goto math;

		/* TODO comparison */
	}
	return NULL;
math:
	return BinaryOperator::Create(instr, lhs.codeGen(context),
		rhs.codeGen(context), "", context.currentBlock());
}

Value* NAssignment::codeGen(CodeGenContext& context)
{
	std::cout << "Creating assignment for " << lhs.name << endl;
	if (context.locals().find(lhs.name) == context.locals().end()) {
		std::cerr << "undeclared variable " << lhs.name << endl;
		return NULL;
	}
	return new StoreInst(rhs.codeGen(context), context.locals()[lhs.name], false, context.currentBlock());
}

Value* NBlock::codeGen(CodeGenContext& context)
{
	StatementList::const_iterator it;
	Value *last = NULL;
	for (it = statements.begin(); it != statements.end(); it++) {
		std::cout << "Generating code for " << typeid(**it).name() << endl;
		last = (**it).codeGen(context);
	}
	std::cout << "Creating block" << endl;
	return last;
}

Value* NExpressionStatement::codeGen(CodeGenContext& context)
{
	std::cout << "Generating code for " << typeid(expression).name() << endl;
	return expression.codeGen(context);
}

Value* NReturnStatement::codeGen(CodeGenContext& context)
{
	std::cout << "Generating return code for " << typeid(expression).name() << endl;
	Value *returnValue = expression.codeGen(context);
	context.setCurrentReturnValue(returnValue);
	return returnValue;
}

Value* NVariableDeclaration::codeGen(CodeGenContext& context)
{
	std::cout << "Creating variable declaration " << type.name << " " << id.name << endl;
	AllocaInst *alloc = new AllocaInst(typeOf(type),4, id.name.c_str(), context.currentBlock());
	context.locals()[id.name] = alloc;
	if (assignmentExpr != NULL) {
		NAssignment assn(id, *assignmentExpr);
		assn.codeGen(context);
	}
	return alloc;
}

Value* NExternDeclaration::codeGen(CodeGenContext& context)
{
	std::vector<Type*> argTypes;
	for (auto it = arguments.begin(); it != arguments.end(); it++) {
		argTypes.push_back(typeOf((**it).type));
	}

	FunctionType *ftype = FunctionType::get(typeOf(type), argTypes, false);

	Function *function = Function::Create(
		ftype,
		GlobalValue::ExternalLinkage,
		id.name,
		context.module
	);

	return function;
}
// -----------------------------------------------------------------------------
Value* NFunctionDeclaration::codeGen(CodeGenContext& context)
{
	vector<Type*> argTypes;
	VariableList::const_iterator it;
	for (it = arguments.begin(); it != arguments.end(); it++) {
		argTypes.push_back(typeOf((**it).type));
	}
	FunctionType *ftype = FunctionType::get(typeOf(type), std::vector<Type*>(argTypes), false);
	Function *function = Function::Create(ftype, GlobalValue::InternalLinkage, id.name.c_str(), context.module);
	BasicBlock *bblock = BasicBlock::Create(MyContext, "entry", function, 0);

	context.pushBlock(bblock);

	Function::arg_iterator argsValues = function->arg_begin();
    Value* argumentValue;

	for (it = arguments.begin(); it != arguments.end(); it++) {
		(**it).codeGen(context);

		argumentValue = &*argsValues++;
		argumentValue->setName((*it)->id.name.c_str());
		StoreInst *inst = new StoreInst(argumentValue, context.locals()[(*it)->id.name], false, bblock);
	}

	block.codeGen(context);
	ReturnInst::Create(MyContext, context.getCurrentReturnValue(), bblock);

	context.popBlock();
	std::cout << "Creating function: " << id.name << endl;
	return function;
}
// -----------------------------------------------------------------------------
Value*  NString::codeGen(CodeGenContext& context)  {
	std::cout << "Creating LLVM String constant: " << value << std::endl;

	llvm::Constant *string_const = llvm::ConstantDataArray::getString(MyContext, value, true);
	llvm::ArrayType* array_type = llvm::ArrayType::get(llvm::Type::getInt8Ty(MyContext), value.length() + 1);
	llvm::GlobalVariable *global_str = new llvm::GlobalVariable(
		*context.module,
		array_type,
		true, //  (read-only)
	llvm::GlobalValue::PrivateLinkage,
	string_const,
	".str.literal"
	);


	return global_str;
}
