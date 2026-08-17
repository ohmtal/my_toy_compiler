#include <iostream>
#include "codegen.h"
#include "node.h"

#include <llvm/Support/DynamicLibrary.h>

using namespace std;

extern void loadRuntimeLibrary();
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
	} else {
		open_file("scripts/main.toy");
	}
	yyparse();
	cout << programBlock << endl;
    // see http://comments.gmane.org/gmane.comp.compilers.llvm.devel/33877
	InitializeNativeTarget();
	InitializeNativeTargetAsmPrinter();
	InitializeNativeTargetAsmParser();

	// add bindings:
	loadRuntimeLibrary();

	CodeGenContext context;
	// // // createCoreFunctions(context);
	context.generateCode(*programBlock);
	context.runCode();
	
	return 0;
}

