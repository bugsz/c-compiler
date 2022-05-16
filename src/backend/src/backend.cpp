#include <unistd.h>
#include <fcntl.h>
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"

#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Utils.h"
#include "llvm/Transforms/Utils/PromoteMemToReg.h"

#include "llvm_utils.h"
#include "frontend.h"
#include "tree_gen.h"

using namespace llvm;

static std::unique_ptr<ExprAST> ast;
void run_lib_backend(int argc, const char **argv) {
    auto frontend_ret = frontend_entry(argc, argv);
    ast = generateBackendAST(frontend_ret);
    print("Finish generating AST for backend");
    ast->codegen();
}

void initializeModule() {
    llvmContext = std::make_unique<LLVMContext>();
    llvmModule = std::make_unique<Module>("JIT", *llvmContext);
    llvmBuilder = std::make_unique<IRBuilder<>>(*llvmContext);
    llvmFPM = std::make_unique<legacy::FunctionPassManager>(llvmModule.get());
    llvmFPM->add(createInstructionCombiningPass());
    llvmFPM->add(createReassociatePass());
    llvmFPM->add(createGVNPass());
    llvmFPM->add(createCFGSimplificationPass());
    llvmFPM->add(llvm::createPromoteMemoryToRegisterPass());
    llvmFPM->add(llvm::createInstructionCombiningPass());
    llvmFPM->add(llvm::createReassociatePass());
    llvmFPM->doInitialization();
}

int compile() {
    InitializeAllTargetInfos();
    InitializeAllTargets();
    InitializeAllTargetMCs();
    InitializeAllAsmParsers();
    InitializeAllAsmPrinters();

    auto targetTriple = sys::getDefaultTargetTriple();

    auto cpu = "generic";
    auto features = "";

    std::string error;
    auto target = TargetRegistry::lookupTarget(targetTriple, error);

    TargetOptions opt;
    auto RM = Optional<Reloc::Model>();
    auto theTargetMachine = target->createTargetMachine(targetTriple, cpu, features, opt, RM);

    llvmModule->setDataLayout(theTargetMachine->createDataLayout());

    auto filename = "output.o";
    std::cout << "Using output filename: " << filename << std::endl; 
    std::error_code EC;
    raw_fd_ostream dest(filename, EC, sys::fs::OF_None);

    legacy::PassManager pass;
    auto FileType = CGFT_ObjectFile;

    if (theTargetMachine->addPassesToEmitFile(pass, dest, nullptr, FileType)) {
        errs() << "TheTargetMachine can't emit a file of this type";
        return 1;
    }

    pass.run(*llvmModule);
    dest.flush();
    return 0;
}

void save() {
    std::string IRText;
    raw_string_ostream OS(IRText);
    OS << *llvmModule;
    OS.flush();
    #ifdef IRONLY
        std::cout << IRText;
    #else
        std::string fileName("output.ll");
        std::ofstream outFile;
        outFile.open(fileName);
        outFile << IRText;
        std::cout << "IR code saved to " + fileName << std::endl;
        outFile.close();
    #endif
}

int main(int argc, const char **argv) {
    #ifdef IRONLY
        int stdout_fd = dup(STDOUT_FILENO);
        close(STDOUT_FILENO);
        int devnull = open("/dev/null", O_WRONLY);
        dup2(devnull, STDOUT_FILENO);
    #endif
    
    initializeModule();
    run_lib_backend(argc, argv);
    // llvmModule->print(errs(), nullptr);

    #ifndef IRONLY
        save();
        int status_code = compile();
        std::cout << "Compile end with status code: " << status_code << std::endl << std::endl;
    #else
        close(devnull);
    	dup2(stdout_fd, STDOUT_FILENO);
        save();
    #endif
    return 0;
}
