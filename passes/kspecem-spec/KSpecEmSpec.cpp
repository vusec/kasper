#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CommandLine.h"

#include <llvm/IR/IRBuilder.h>
#include "llvm/IR/InstVisitor.h"

#include "llvm/Transforms/IPO/PassManagerBuilder.h" // RegisterStandardPasses

#include <kspecem/KSpecEmABIList.h>

#define DEBUG_TYPE "kspecem"
#define DEBUG LLVM_DEBUG

#define NEW_CHECKPOINT_FUNC_NAME "kspecem_hook_new_checkpoint"
#define SAVE_STACK_FUNC_NAME "kspecem_hook_save_stack"
#define LEAVE_RT_FUNC_NAME "kspecem_hook_leave_rt"

using namespace llvm;

cl::list<std::string> kspecemSpecABIList("kspecem-spec-abilist",
    cl::desc("Specify the checkpointing method to use."), cl::CommaSeparated, cl::Hidden);

cl::opt<std::string> kspecemSpecFunctionPrefix("kspecem-spec-func-prefix",
    cl::desc("Specify the possible prefix functions can have."));

namespace {

class KSpecEmSpec {
  friend struct KSpecEmSpecFunction;
  friend class KSpecEmSpecPassVisitor;

  Module *Mod;
  LLVMContext *Ctx;

  KSpecEmABIList kspecem_functions;
  std::string funcPrefix;

  Function *newCheckpointHook;
  Function *saveStackHook;
  Function *saveRegistersHook;
  Function *reenableIrqsHook;
  Function *checkCallDepthHook;

  bool init(Module &M);
  bool visitor(Function &F);

public:
  bool runImpl(Module &M);
};

struct KSpecEmSpecFunction {
  KSpecEmSpec &LS;
  Function *F;

  KSpecEmSpecFunction(KSpecEmSpec &LS, Function *F): LS(LS), F(F) {}

  void insertNewCheckpointHook(IRBuilder<> &IRB);
};

class KSpecEmSpecPassVisitor : public InstVisitor<KSpecEmSpecPassVisitor> {
public:
  KSpecEmSpecFunction &LSF;

  KSpecEmSpecPassVisitor(KSpecEmSpecFunction &LSF) : LSF(LSF) {}

  void visitBranchInst(BranchInst &BI);
};

bool KSpecEmSpec::init(Module &M) {
  Mod = &M;
  Ctx = &M.getContext();

  kspecem_functions.set(SpecialCaseList::createOrDie(kspecemSpecABIList,
        *vfs::getRealFileSystem()), kspecemSpecFunctionPrefix);
  funcPrefix = kspecemSpecFunctionPrefix;

  std::vector<Type*> newCheckpointParamTypes = {};
  Type *newCheckpointRetType = Type::getInt64Ty(*Ctx);
  FunctionType *newCheckpointFuncType = FunctionType::get(newCheckpointRetType,
      newCheckpointParamTypes, false);
  Value *newCheckpointFunc  = Mod->getOrInsertFunction(NEW_CHECKPOINT_FUNC_NAME,
      newCheckpointFuncType).getCallee();
  if (newCheckpointFunc == NULL) {
    return false;
  }
  newCheckpointHook = cast<Function>(newCheckpointFunc);
  newCheckpointHook->setCallingConv(CallingConv::Fast);

  std::vector<Type*> saveRegistersParamTypes = {
    Type::getInt8PtrTy(*Ctx), Type::getInt64Ty(*Ctx)
  };
  Type *saveRegistersRetType = Type::getVoidTy(*Ctx);
  FunctionType *saveRegistersFuncType = FunctionType::get(saveRegistersRetType,
      saveRegistersParamTypes, false);
  Value *saveRegistersFunc = Mod->getOrInsertFunction("kspecem_asm_save_registers",
      saveRegistersFuncType).getCallee();
  if (saveRegistersFunc == NULL) {
    return false;
  }
  saveRegistersHook = cast<Function>(saveRegistersFunc);
  saveRegistersHook->setCallingConv(CallingConv::Fast);

  std::vector<Type*> saveStackParamTypes = {
    Type::getInt64Ty(*Ctx), Type::getInt8PtrTy(*Ctx)
  };
  Type *saveStackRetType = Type::getVoidTy(*Ctx);
  FunctionType *saveStackFuncType = FunctionType::get(saveStackRetType,
      saveStackParamTypes, false);
  Value *saveStackFunc = Mod->getOrInsertFunction(SAVE_STACK_FUNC_NAME,
      saveStackFuncType).getCallee();
  if (saveStackFunc == NULL) {
    return false;
  }
  saveStackHook = cast<Function>(saveStackFunc);
  saveStackHook->setCallingConv(CallingConv::Fast);

  std::vector<Type*> reenableIrqsParamTypes = {Type::getInt64Ty(*Ctx)};
  Type *reenableIrqsRetType = Type::getVoidTy(*Ctx);
  FunctionType *reenableIrqsFuncType = FunctionType::get(reenableIrqsRetType,
      reenableIrqsParamTypes, false);
  Value *reenableIrqsFunc = Mod->getOrInsertFunction(LEAVE_RT_FUNC_NAME,
      reenableIrqsFuncType).getCallee();
  if (reenableIrqsFunc == NULL) {
    return false;
  }
  reenableIrqsHook = cast<Function>(reenableIrqsFunc);
  reenableIrqsHook->setCallingConv(CallingConv::Fast);

  std::vector<Type*> checkCallDepthParamTypes = {
    Type::getInt8PtrTy(*Ctx), Type::getInt8PtrTy(*Ctx)
  };
  Type *checkCallDepthRetType = Type::getVoidTy(*Ctx);
  FunctionType *checkCallDepthFuncType = FunctionType::get(checkCallDepthRetType,
      checkCallDepthParamTypes, false);
  Value *checkCallDepthFunc  = Mod->getOrInsertFunction("kspecem_hook_check_call_depth",
      checkCallDepthFuncType).getCallee();
  if (checkCallDepthFunc == NULL) {
    return false;
  }
  checkCallDepthHook = cast<Function>(checkCallDepthFunc);
  checkCallDepthHook->setCallingConv(CallingConv::Fast);

  return true;
}

bool KSpecEmSpec::visitor(Function &F) {
  bool Changed = false;

  if (kspecem_functions.isIn(F, "blacklist")
      || kspecem_functions.isIn(F, "exception")
      || kspecem_functions.isIn(F, "timer")
      || kspecem_functions.isIn(F, "nospec")
    ) {
    return Changed;
  }

  /* insert call depth hook in the beginning of the function */
  for (Function::iterator it = F.begin(); it != F.end(); ++it) {
    BasicBlock *bb = &*it;
    for (BasicBlock::iterator it2 = bb->begin(); it2 != bb->end(); ++it2) {
      Instruction *inst = &*it2;
      if (isa<PHINode>(inst))
        continue;

      std::vector<Value*> checkCallDepthArgs(2);
      IRBuilder<> builder(*Ctx);
      builder.SetInsertPoint(inst);

      Value *callInst = builder.CreatePointerCast(&F, llvm::Type::getInt8PtrTy(*Ctx));

      std::vector<Value*> returnAddressArgs(1);
      returnAddressArgs[0] = builder.getInt32(0);
      Function *fun = Intrinsic::getDeclaration(Mod, Intrinsic::returnaddress);
      CallInst *returnAddress = builder.CreateCall(fun, returnAddressArgs);

      checkCallDepthArgs[0] = callInst;
      checkCallDepthArgs[1] = returnAddress;

      builder.CreateCall(checkCallDepthHook, checkCallDepthArgs);
      Changed = true;
      break;
    }
    break;
  }

  KSpecEmSpecFunction LSF(*this, &F);
  KSpecEmSpecPassVisitor(LSF).visit(F);
  Changed = true;

  return Changed;
}

bool KSpecEmSpec::runImpl(Module &M) {
  if (M.getName().contains("libkspecem.a"))
    return false;
  if (!init(M))
    return false;

  bool Changed = false;
  for (Function &F : M)
    Changed |= visitor(F);
  return true;
}

void KSpecEmSpecFunction::insertNewCheckpointHook(IRBuilder<> &IRB) {
  std::vector<Value*> args(0);
  CallInst *CI = IRB.CreateCall(LS.newCheckpointHook, args);

  std::vector<Value*> frameAddressArgs(1);
  frameAddressArgs[0] = ConstantInt::get(Type::getInt32Ty(*LS.Ctx), 0);
  Function *fun = Intrinsic::getDeclaration(LS.Mod, Intrinsic::frameaddress,
      IRB.getInt8PtrTy(LS.Mod->getDataLayout().getAllocaAddrSpace()));
  CallInst *frameAddress = IRB.CreateCall(fun, frameAddressArgs);

  std::vector<Value*> saveStackArgs(2);
  saveStackArgs[0] = CI;
  saveStackArgs[1] = frameAddress;
  IRB.CreateCall(LS.saveStackHook, saveStackArgs);

  DEBUG(errs() << "saveRegistersHook: " << LS.saveRegistersHook << "\n");
  std::vector<Value*> saveRegistersArgs(2);
  Constant *var = LS.Mod->getOrInsertGlobal("kspecem_registers_ptr",
      Type::getInt64Ty(*LS.Ctx));
  Value *kspecemRegistersVal = IRB.CreateLoad(Type::getInt8PtrTy(*LS.Ctx), var);

  saveRegistersArgs[0] = kspecemRegistersVal;
  saveRegistersArgs[1] = CI;
  IRB.CreateCall(LS.saveRegistersHook, saveRegistersArgs);

  std::vector<Value*> reenableIrqsArgs(1);
  reenableIrqsArgs[0] = CI;
  IRB.CreateCall(LS.reenableIrqsHook, reenableIrqsArgs);
}

void KSpecEmSpecPassVisitor::visitBranchInst(BranchInst &BI) {
	if (!BI.isConditional())
    return;

  Constant *var = LSF.LS.Mod->getOrInsertGlobal("kspecem_swap_branch_condition",
      Type::getInt32Ty(*LSF.LS.Ctx));

  IRBuilder<> IRB(*LSF.LS.Ctx);
  IRB.SetInsertPoint(&BI);

  LSF.insertNewCheckpointHook(IRB);

  Value *condition = BI.getCondition();
  Value *invertedCondition;

  if (isa<CmpInst>(condition)) {
    CmpInst *cond = cast<CmpInst>(condition);
    invertedCondition = (CmpInst::isFPPredicate(cond->getPredicate()) ?
        IRB.CreateFCmp(cond->getInversePredicate(), cond->getOperand(0), cond->getOperand(1)) :
        IRB.CreateICmp(cond->getInversePredicate(), cond->getOperand(0), cond->getOperand(1)));
  } else {
    invertedCondition = IRB.CreateNot(condition);
  }
  Value *swapBranchCondition = IRB.CreateLoad(Type::getInt32Ty(*LSF.LS.Ctx), var);
  Value *cond = IRB.CreateICmpEQ(swapBranchCondition, IRB.getInt32(1));
  Value *newCondition = IRB.CreateSelect(cond,
      invertedCondition, condition);
  IRB.CreateStore(IRB.getInt32(0), var);
  BI.setCondition(newCondition);
}

// New PM implementation
struct KSpecEmSpecPass : PassInfoMixin<KSpecEmSpecPass> {
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &) {
    bool Changed = KSpecEmSpec().runImpl(M);
    return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
  }
};

// Legacy PM implementation
struct LegacyKSpecEmSpecPass : public ModulePass {
  static char ID;
  LegacyKSpecEmSpecPass() : ModulePass(ID) {}
  // Main entry point - the name conveys what unit of IR this is to be run on.
  bool runOnModule(Module &M) override {
    return KSpecEmSpec().runImpl(M);
  }
};
} // namespace

//-----------------------------------------------------------------------------
// New PM Registration
//-----------------------------------------------------------------------------
llvm::PassPluginLibraryInfo getKSpecEmSpecPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "KSpecEmSpec", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerOptimizerLastEPCallback(
                [](llvm::ModulePassManager &PM,
                  llvm::PassBuilder::OptimizationLevel Level) {
                PM.addPass(KSpecEmSpecPass());
                });
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "kspecem-spec") {
                    MPM.addPass(KSpecEmSpecPass());
                    return true;
                  }
                  return false;
                });
          }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getKSpecEmSpecPassPluginInfo();
}

//-----------------------------------------------------------------------------
// Legacy PM Registration
//-----------------------------------------------------------------------------
// The address of this variable is used to uniquely identify the pass. The
// actual value doesn't matter.
char LegacyKSpecEmSpecPass::ID = 0;

static RegisterPass<LegacyKSpecEmSpecPass>
    X("legacy-kspecem-spec", "KSpecEm Spec Pass",
      false, // This pass DOES modify the CFG => false
      false // This pass is not a pure analysis pass => false
    );

static llvm::RegisterStandardPasses RegisterKSpecEmSpecLTOThinPass(
    llvm::PassManagerBuilder::EP_OptimizerLast,
    [](const llvm::PassManagerBuilder &Builder,
       llvm::legacy::PassManagerBase &PM) { PM.add(new LegacyKSpecEmSpecPass()); });

static llvm::RegisterStandardPasses RegisterKSpecEmSpecLTOPass(
    llvm::PassManagerBuilder::EP_FullLinkTimeOptimizationLast,
    [](const llvm::PassManagerBuilder &Builder,
       llvm::legacy::PassManagerBase &PM) { PM.add(new LegacyKSpecEmSpecPass()); });
