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

int compile(std::string filename) {
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

int backend_entry(lib_frontend_ret ret, std::string filename) {
    initializeModule();
    ast = generateBackendAST(ret);
    ast->codegen();
    return compile(filename);
}
