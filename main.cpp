#include <iostream>
#include "codegen.h"
#include "node.h"

#include <llvm/Support/DynamicLibrary.h>

using namespace std;

extern "C" {
	void printi(int val);
}

extern int yyparse();
extern NBlock* programBlock;

void open_file(const char* filename) {
	// openfile
	freopen(filename, "r", stdin);
}

void createCoreFunctions(CodeGenContext& context);

int main(int argc, char **argv)
{
	if (argc > 1) {
		open_file(argv[1]);
	}
	yyparse();
	cout << programBlock << endl;
    // see http://comments.gmane.org/gmane.comp.compilers.llvm.devel/33877
	InitializeNativeTarget();
	InitializeNativeTargetAsmPrinter();
	InitializeNativeTargetAsmParser();

	//FIXME need something better
	// i add the new extern functions here :::
	llvm::sys::DynamicLibrary::AddSymbol("printi", (void*)&printi);

	CodeGenContext context;
	createCoreFunctions(context);
	context.generateCode(*programBlock);
	context.runCode();
	
	return 0;
}

