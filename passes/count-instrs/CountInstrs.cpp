#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CommandLine.h"

#include <llvm/IR/IRBuilder.h>
#include "llvm/IR/InstVisitor.h"

#include "llvm/Transforms/IPO/PassManagerBuilder.h" // RegisterStandardPasses
#include <llvm/Transforms/Utils/BasicBlockUtils.h> // SplitBlock

using namespace llvm;

namespace {

class CountInstrs {
  Module *Mod;
  LLVMContext *Ctx;

  bool init(Module &M);
  bool visitor(Function &F);

public:
  bool runImpl(Module &M);
};

bool CountInstrs::init(Module &M) {
  Mod = &M;
  Ctx = &M.getContext();

  return true;
}

bool CountInstrs::visitor(Function &F) {
  bool Changed = false;
  int count = 0;
  for (BasicBlock &BB : F) {
    for (Instruction &I: BB) {
      count++;
    }
  }
  if (count)
    errs() << "KASPER: {\"" << F.getName() << "\": " << count << "}\n";
  return Changed;
}

bool CountInstrs::runImpl(Module &M) {
  bool Changed = false;
  for (Function &F : M)
    Changed |= visitor(F);
  return true;
}

// New PM implementation
struct CountInstrsPass : PassInfoMixin<CountInstrsPass> {
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &) {
    bool Changed = CountInstrs().runImpl(M);
    return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
  }
};

// Legacy PM implementation
struct LegacyCountInstrsPass : public ModulePass {
  static char ID;
  LegacyCountInstrsPass() : ModulePass(ID) {}
  // Main entry point - the name conveys what unit of IR this is to be run on.
  bool runOnModule(Module &M) override {
    return CountInstrs().runImpl(M);
  }
};
} // namespace

//-----------------------------------------------------------------------------
// New PM Registration
//-----------------------------------------------------------------------------
llvm::PassPluginLibraryInfo getCountInstrsPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "CountInstrs", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerOptimizerLastEPCallback(
                [](llvm::ModulePassManager &PM,
                  llvm::PassBuilder::OptimizationLevel Level) {
                PM.addPass(CountInstrsPass());
                });
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "count-instrs") {
                    MPM.addPass(CountInstrsPass());
                    return true;
                  }
                  return false;
                });
          }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getCountInstrsPassPluginInfo();
}

//-----------------------------------------------------------------------------
// Legacy PM Registration
//-----------------------------------------------------------------------------
// The address of this variable is used to uniquely identify the pass. The
// actual value doesn't matter.
char LegacyCountInstrsPass::ID = 0;

static RegisterPass<LegacyCountInstrsPass>
    X("legacy-count-instrs", "CountInstrs Pass",
      false, // This pass DOES modify the CFG => false
      false // This pass is not a pure analysis pass => false
    );

static llvm::RegisterStandardPasses RegisterCountInstrsLTOThinPass(
    llvm::PassManagerBuilder::EP_OptimizerLast,
    [](const llvm::PassManagerBuilder &Builder,
       llvm::legacy::PassManagerBase &PM) { PM.add(new LegacyCountInstrsPass()); });

static llvm::RegisterStandardPasses RegisterCountInstrsLTOPass(
    llvm::PassManagerBuilder::EP_FullLinkTimeOptimizationLast,
    [](const llvm::PassManagerBuilder &Builder,
       llvm::legacy::PassManagerBase &PM) { PM.add(new LegacyCountInstrsPass()); });
