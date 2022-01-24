#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

#include <llvm/IR/IRBuilder.h>
#include "llvm/IR/InstVisitor.h"

#include "llvm/Transforms/IPO/PassManagerBuilder.h" // RegisterStandardPasses

using namespace llvm;

namespace {

class KSpecEmPfChecker {
  friend struct KSpecEmPfCheckerFunction;
  friend class KSpecEmPfCheckerPassVisitor;

  Module *Mod;
  LLVMContext *Ctx;

  FunctionCallee AccessHookFn;

  bool init(Module &M);
  bool visitor(Function &F);

public:
  bool runImpl(Module &M);
};

struct KSpecEmPfCheckerFunction {
  KSpecEmPfChecker &KPC;
  Function *F;

  KSpecEmPfCheckerFunction(KSpecEmPfChecker &KPC, Function *F): KPC(KPC), F(F) {}
};

class KSpecEmPfCheckerPassVisitor : public InstVisitor<KSpecEmPfCheckerPassVisitor> {
public:
  KSpecEmPfCheckerFunction &KPCF;

  KSpecEmPfCheckerPassVisitor(KSpecEmPfCheckerFunction &KPCF) : KPCF(KPCF) {}

  void visitLoadInst(LoadInst &LI);
  void visitStoreInst(StoreInst &SI);
  void visitMemIntrinsic(MemIntrinsic &MI);
};

bool KSpecEmPfChecker::init(Module &M) {
  Mod = &M;
  Ctx = &M.getContext();

  auto &DL = Mod->getDataLayout();
  IntegerType *SizeTy = DL.getIntPtrType(*Ctx);

  Type *AccessHookArgs[2] = { Type::getInt8PtrTy(*Ctx), SizeTy };
  FunctionType *AccessHookFnTy = FunctionType::get(
      Type::getInt8PtrTy(*Ctx), AccessHookArgs, /*isVarArg=*/false);
  AccessHookFn = Mod->getOrInsertFunction("kspecem_hook_access", AccessHookFnTy);

  return true;
}

bool KSpecEmPfChecker::visitor(Function &F) {
  bool Changed = false;
  KSpecEmPfCheckerFunction KPCF(*this, &F);
  KSpecEmPfCheckerPassVisitor(KPCF).visit(F);
  Changed = true;
  return Changed;
}

bool KSpecEmPfChecker::runImpl(Module &M) {
  if (M.getName().contains("libkspecem.a"))
    return false;
  if (!init(M))
    return false;

  bool Changed = false;
  for (Function &F : M)
    Changed |= visitor(F);
  return true;
}

void KSpecEmPfCheckerPassVisitor::visitLoadInst(LoadInst &LI) {
  if(!LI.getMetadata("is-asan-instr")) return;

  auto &DL = LI.getModule()->getDataLayout();
  IntegerType *SizeTy = DL.getIntPtrType(*KPCF.KPC.Ctx);

  IRBuilder<> IRB(&LI);
  Value *Ptr = IRB.CreateBitCast(LI.getPointerOperand(), Type::getInt8PtrTy(*KPCF.KPC.Ctx));
  Value *Size = ConstantInt::get(SizeTy, DL.getTypeStoreSize(LI.getType()));
  CallInst *Call = IRB.CreateCall(KPCF.KPC.AccessHookFn, {Ptr, Size});
  Value *NewPtr = IRB.CreateBitCast(Call, LI.getPointerOperandType());
  LI.setOperand(0, NewPtr); // TODO: There's probably a better way of setting the pointer operand
}

void KSpecEmPfCheckerPassVisitor::visitStoreInst(StoreInst &SI) {
  if(!SI.getMetadata("is-asan-instr")) return;

  auto &DL = SI.getModule()->getDataLayout();
  IntegerType *SizeTy = DL.getIntPtrType(*KPCF.KPC.Ctx);

  IRBuilder<> IRB(&SI);
  Value *Ptr = IRB.CreateBitCast(SI.getPointerOperand(), Type::getInt8PtrTy(*KPCF.KPC.Ctx));
  Value *Size = ConstantInt::get(SizeTy, DL.getTypeStoreSize(SI.getValueOperand()->getType()));
  CallInst *Call = IRB.CreateCall(KPCF.KPC.AccessHookFn, {Ptr, Size});
  Value *NewPtr = IRB.CreateBitCast(Call, SI.getPointerOperandType());
  SI.setOperand(1, NewPtr); // TODO: There's probably a better way of setting the pointer operand
}

void KSpecEmPfCheckerPassVisitor::visitMemIntrinsic(MemIntrinsic &MI) {
  if(!MI.getMetadata("is-asan-instr")) return;

  auto &DL = MI.getModule()->getDataLayout();
  IntegerType *SizeTy = DL.getIntPtrType(*KPCF.KPC.Ctx);

  IRBuilder<> IRB(&MI);
  Value *Ptr = IRB.CreateBitCast(MI.getDest(), Type::getInt8PtrTy(*KPCF.KPC.Ctx));
  Value *Size = IRB.CreateZExtOrTrunc(MI.getLength(), SizeTy);
  CallInst *Call = IRB.CreateCall(KPCF.KPC.AccessHookFn, {Ptr, Size});
  MI.setDest(Call); // TODO: Are we guaranteed that the destination of a memintrinsic is an Int8PtrTy?
}

// New PM implementation
struct KSpecEmPfCheckerPass : PassInfoMixin<KSpecEmPfCheckerPass> {
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &) {
    bool Changed = KSpecEmPfChecker().runImpl(M);
    return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
  }
};

// Legacy PM implementation
struct LegacyKSpecEmPfCheckerPass : public ModulePass {
  static char ID;
  LegacyKSpecEmPfCheckerPass() : ModulePass(ID) {}
  // Main entry point - the name conveys what unit of IR this is to be run on.
  bool runOnModule(Module &M) override {
    return KSpecEmPfChecker().runImpl(M);
  }
};
} // namespace

//-----------------------------------------------------------------------------
// New PM Registration
//-----------------------------------------------------------------------------
llvm::PassPluginLibraryInfo getKSpecEmPfCheckerPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "KSpecEmPfChecker", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerOptimizerLastEPCallback(
                [](llvm::ModulePassManager &PM,
                  llvm::PassBuilder::OptimizationLevel Level) {
                PM.addPass(KSpecEmPfCheckerPass());
                });
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "kspecem-pf-checker") {
                    MPM.addPass(KSpecEmPfCheckerPass());
                    return true;
                  }
                  return false;
                });
          }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getKSpecEmPfCheckerPassPluginInfo();
}

//-----------------------------------------------------------------------------
// Legacy PM Registration
//-----------------------------------------------------------------------------
// The address of this variable is used to uniquely identify the pass. The
// actual value doesn't matter.
char LegacyKSpecEmPfCheckerPass::ID = 0;

static RegisterPass<LegacyKSpecEmPfCheckerPass>
    X("legacy-kspecem-pf-checker", "KSpecEm Pf Checker Pass",
      false, // This pass DOES modify the CFG => false
      false // This pass is not a pure analysis pass => false
    );

static llvm::RegisterStandardPasses RegisterKSpecEmPfCheckerLTOThinPass(
    llvm::PassManagerBuilder::EP_OptimizerLast,
    [](const llvm::PassManagerBuilder &Builder,
       llvm::legacy::PassManagerBase &PM) { PM.add(new LegacyKSpecEmPfCheckerPass()); });

static llvm::RegisterStandardPasses RegisterKSpecEmPfCheckerLTOPass(
    llvm::PassManagerBuilder::EP_FullLinkTimeOptimizationLast,
    [](const llvm::PassManagerBuilder &Builder,
       llvm::legacy::PassManagerBase &PM) { PM.add(new LegacyKSpecEmPfCheckerPass()); });
