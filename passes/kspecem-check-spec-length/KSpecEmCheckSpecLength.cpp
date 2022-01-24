#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

#include <llvm/IR/IRBuilder.h>

#include "llvm/Transforms/IPO/PassManagerBuilder.h" // RegisterStandardPasses

using namespace llvm;

namespace {

class KSpecEmCheckSpecLength {
  Module *Mod;
  LLVMContext *Ctx;

  Function *kspecemCheckSpecLengthHook;

  bool init(Module &M);
  bool visitor(Function &F);

public:
  bool runImpl(Module &M);
};

bool KSpecEmCheckSpecLength::init(Module &M) {
  Mod = &M;
  Ctx = &M.getContext();

  std::vector<Type*> kspecemCheckSpecLengthFuncParamTypes = {Type::getInt32Ty(*Ctx)};
  Type *kspecemCheckSpecLengthFuncRetType = Type::getVoidTy(*Ctx);
  FunctionType *kspecemCheckSpecLengthFuncType = FunctionType::get(
      kspecemCheckSpecLengthFuncRetType, kspecemCheckSpecLengthFuncParamTypes, false);
  Value *kspecemCheckSpecLengthFunc = Mod->getOrInsertFunction(
      "kspecem_hook_check_spec_length",
      kspecemCheckSpecLengthFuncType).getCallee();
  if (kspecemCheckSpecLengthFunc == NULL) {
    errs() << "Could not getOrInsertFunction!\n";
    return false;
  }
  kspecemCheckSpecLengthHook = cast<Function>(kspecemCheckSpecLengthFunc);
  kspecemCheckSpecLengthHook->setCallingConv(CallingConv::Fast);
  return true;
}

bool KSpecEmCheckSpecLength::visitor(Function &F) {
  bool Changed = false;
  if (F.getName().startswith("__dfsan")) {
    return Changed;
  }
  if (F.getName().equals("stop_nmi")) {
    return Changed;
  }
  if (F.getName().equals("unset_rt")) {
    return Changed;
  }
  if (F.getName().startswith("kspecem_")) {
    return Changed;
  }

  for (Function::iterator it = F.begin(); it != F.end(); ++it) {
    BasicBlock *bb = &*it;
    unsigned int bbInstCount = std::distance(bb->begin(), bb->end());

    Instruction *inst = bb->getTerminator();

    IRBuilder<> builder(*Ctx);
    builder.SetInsertPoint(inst);

    std::vector<Value*> args(1);
    args[0] = ConstantInt::get(Type::getInt32Ty(*Ctx), bbInstCount);
    builder.CreateCall(kspecemCheckSpecLengthHook, args);
    Changed = true;
  }
  return Changed;
}

bool KSpecEmCheckSpecLength::runImpl(Module &M) {
  if (M.getName().contains("libkspecem.a"))
    return false;
  if ((M.getName().contains("kdfsan") || M.getName().contains("kdfinit"))
      && !M.getName().contains("test_kdfsan"))
    return false;
  if (!init(M))
    return false;

  bool Changed = false;
  for (Function &F : M)
    Changed |= visitor(F);
  return true;
}


// New PM implementation
struct KSpecEmCheckSpecLengthPass : PassInfoMixin<KSpecEmCheckSpecLengthPass> {
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &) {
    bool Changed = KSpecEmCheckSpecLength().runImpl(M);
    return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
  }
};

// Legacy PM implementation
struct LegacyKSpecEmCheckSpecLengthPass : public ModulePass {
  static char ID;
  LegacyKSpecEmCheckSpecLengthPass() : ModulePass(ID) {}
  // Main entry point - the name conveys what unit of IR this is to be run on.
  bool runOnModule(Module &M) override {
    return KSpecEmCheckSpecLength().runImpl(M);
  }
};
} // namespace

//-----------------------------------------------------------------------------
// New PM Registration
//-----------------------------------------------------------------------------
llvm::PassPluginLibraryInfo getKSpecEmCheckSpecLengthPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "KSpecEmCheckSpecLength", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerOptimizerLastEPCallback(
                [](llvm::ModulePassManager &PM,
                  llvm::PassBuilder::OptimizationLevel Level) {
                PM.addPass(KSpecEmCheckSpecLengthPass());
                });
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "kspecem-check-spec-length") {
                    MPM.addPass(KSpecEmCheckSpecLengthPass());
                    return true;
                  }
                  return false;
                });
          }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getKSpecEmCheckSpecLengthPassPluginInfo();
}

//-----------------------------------------------------------------------------
// Legacy PM Registration
//-----------------------------------------------------------------------------
// The address of this variable is used to uniquely identify the pass. The
// actual value doesn't matter.
char LegacyKSpecEmCheckSpecLengthPass::ID = 0;

static RegisterPass<LegacyKSpecEmCheckSpecLengthPass>
    X("legacy-kspecem-check-spec-length", "KSpecEm Check Spec Length Pass",
      false, // This pass DOES modify the CFG => false
      false // This pass is not a pure analysis pass => false
    );

static llvm::RegisterStandardPasses RegisterKSpecEmCheckSpecLengthLTOThinPass(
    llvm::PassManagerBuilder::EP_OptimizerLast,
    [](const llvm::PassManagerBuilder &Builder,
       llvm::legacy::PassManagerBase &PM) { PM.add(new LegacyKSpecEmCheckSpecLengthPass()); });

static llvm::RegisterStandardPasses RegisterKSpecEmCheckSpecLengthLTOPass(
    llvm::PassManagerBuilder::EP_FullLinkTimeOptimizationLast,
    [](const llvm::PassManagerBuilder &Builder,
       llvm::legacy::PassManagerBase &PM) { PM.add(new LegacyKSpecEmCheckSpecLengthPass()); });
