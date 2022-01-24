#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CommandLine.h"

#include <llvm/IR/IRBuilder.h>
#include "llvm/IR/InstVisitor.h"

#include "llvm/Transforms/IPO/PassManagerBuilder.h" // RegisterStandardPasses
#include <llvm/Transforms/Utils/BasicBlockUtils.h> // SplitBlock

#include <kspecem/KSpecEmABIList.h>

#define RESTART_FUNC_NAME "kspecem_hook_restart"
#define TIMER_INTERRUPT_FUNC_NAME "kspecem_hook_interrupt_timer"
#define EXCEPTION_INTERRUPT_FUNC_NAME "kspecem_hook_interrupt_exception"

#define STORE_HOOK_NAME "kspecem_hook_store"
#define MEMCPY_FUNC_NAME  "kspecem_hook_memcpy"

using namespace llvm;

cl::list<std::string> kspecemABIList("kspecem-abilist",
    cl::desc("Specify the checkpointing method to use."), cl::CommaSeparated, cl::Hidden);

cl::opt<std::string> kspecemFunctionPrefix("kspecem-func-prefix",
    cl::desc("Specify the possible prefix functions can have."));

namespace {

class KSpecEm {
  friend struct KSpecEmFunction;
  friend class KSpecEmPassVisitor;

  Module *Mod;
  LLVMContext *Ctx;

  KSpecEmABIList kspecem_functions;
  std::string funcPrefix;

  Function *storeInstHook;
  Function *memcpyHook;
  Function *restartHook;
  Function *timerInterruptHook;
  Function *exceptionInterruptHook;

  bool init(Module &M);
  bool visitor(Function &F);

public:
  bool runImpl(Module &M);
};

struct KSpecEmFunction {
  KSpecEm &L;
  Function *F;

  KSpecEmFunction(KSpecEm &L, Function *F): L(L), F(F) {}

  Function *getCalledFunction(CallInst &CI,
      int *return_type, Value **calledValue);
  void insertTimerInterruptHook(IRBuilder<> &IRB);
  Value *insertExceptionInterruptHook(IRBuilder<> &IRB);
  void insertRestartHook(IRBuilder<> &IRB, int restart_type, Value *calledFunc);
};

class KSpecEmPassVisitor : public InstVisitor<KSpecEmPassVisitor> {
public:
  KSpecEmFunction &LF;

  KSpecEmPassVisitor(KSpecEmFunction &LF) : LF(LF) {}

  void visitCallInst(CallInst &CI);
  void visitReturnInst(ReturnInst &RI);
  void visitStoreInst(StoreInst &SI);
  void visitMemIntrinsic(MemIntrinsic &MI);
};

bool KSpecEm::init(Module &M) {
  Mod = &M;
  Ctx = &M.getContext();

  kspecem_functions.set(SpecialCaseList::createOrDie(kspecemABIList,
        *vfs::getRealFileSystem()), kspecemFunctionPrefix);
  funcPrefix = kspecemFunctionPrefix;

  std::vector<Type*> storeInstParamTypes = {Type::getInt8PtrTy(*Ctx)};
  Type *storeInstRetType = Type::getVoidTy(*Ctx);
  FunctionType *storeInstFuncType = FunctionType::get(storeInstRetType,
      storeInstParamTypes, false);
	Value *storeInstFunc = M.getOrInsertFunction(STORE_HOOK_NAME,
      storeInstFuncType).getCallee();
	if(storeInstFunc == NULL) {
    return false;
  }
	storeInstHook = cast<Function>(storeInstFunc);
	storeInstHook->setCallingConv(CallingConv::Fast);

  std::vector<Type*> memcpyParamTypes = {
    Type::getInt8PtrTy(*Ctx), Type::getInt64Ty(*Ctx)
  };
  Type *memcpyRetType = Type::getVoidTy(*Ctx);
  FunctionType *memcpyFuncType = FunctionType::get(memcpyRetType,
      memcpyParamTypes, false);
	Value *memcpyFunc  = Mod->getOrInsertFunction(MEMCPY_FUNC_NAME,
      memcpyFuncType).getCallee();
	if(memcpyFunc == NULL) {
    return false;
  }
	memcpyHook = cast<Function>(memcpyFunc);
	memcpyHook->setCallingConv(CallingConv::Fast);

  std::vector<Type*> restartParamTypes = {
    Type::getInt8PtrTy(*Ctx), Type::getInt32Ty(*Ctx),
    Type::getInt8PtrTy(*Ctx)
  };
  Type *restartRetType = Type::getInt32Ty(*Ctx);
  FunctionType *restartFuncType = FunctionType::get(restartRetType,
      restartParamTypes, false);
  Value *restartFunc  = Mod->getOrInsertFunction(RESTART_FUNC_NAME,
      restartFuncType).getCallee();
  if (restartFunc == NULL) {
    return false;
  }
  restartHook = cast<Function>(restartFunc);
  restartHook->setCallingConv(CallingConv::Fast);

  std::vector<Type*> timerInterruptParamTypes = {
    Type::getInt8PtrTy(*Ctx), Type::getInt8PtrTy(*Ctx)
  };
  Type *timerInterruptRetType = Type::getVoidTy(*Ctx);
  FunctionType *timerInterruptFuncType = FunctionType::get(timerInterruptRetType,
      timerInterruptParamTypes, false);
  Value *timerInterruptFunc  = Mod->getOrInsertFunction(TIMER_INTERRUPT_FUNC_NAME,
      timerInterruptFuncType).getCallee();
  if (timerInterruptFunc == NULL) {
    return false;
  }
  timerInterruptHook = cast<Function>(timerInterruptFunc);
  timerInterruptHook->setCallingConv(CallingConv::Fast);

  std::vector<Type*> exceptionInterruptParamTypes = {
    Type::getInt8PtrTy(*Ctx), Type::getInt8PtrTy(*Ctx),
    Type::getInt32Ty(*Ctx)
  };
  Type *exceptionInterruptRetType = Type::getInt32Ty(*Ctx);
  FunctionType *exceptionInterruptFuncType = FunctionType::get(exceptionInterruptRetType,
      exceptionInterruptParamTypes, false);
  Value *exceptionInterruptFunc  = Mod->getOrInsertFunction(EXCEPTION_INTERRUPT_FUNC_NAME,
      exceptionInterruptFuncType).getCallee();
  if (exceptionInterruptFunc == NULL) {
    return false;
  }
  exceptionInterruptHook = cast<Function>(exceptionInterruptFunc);
  exceptionInterruptHook->setCallingConv(CallingConv::Fast);

  return true;
}

bool KSpecEm::visitor(Function &F) {
  bool Changed = false;

  if (kspecem_functions.isIn(F, "blacklist")) {
    for (Function::iterator it = F.begin(); it != F.end(); ++it) {
      BasicBlock *bb = &*it;
      for (BasicBlock::iterator it2 = bb->begin(); it2 != bb->end(); ++it2) {
        Instruction *inst = &*it2;

        IRBuilder<> IRB(*Ctx);
        IRB.SetInsertPoint(inst);
        KSpecEmFunction LF(*this, &F);
        LF.insertRestartHook(IRB, 4, &F);
        Changed = true;
        break;
      }
      break;
    }
    return Changed;
  }

  if (kspecem_functions.isIn(F, "timer")) {
    BasicBlock *bb = &F.getEntryBlock();
    Instruction *inst = bb->getFirstNonPHI();

    IRBuilder<> IRB(*Ctx);
    IRB.SetInsertPoint(inst);
    KSpecEmFunction LF(*this, &F);
    LF.insertTimerInterruptHook(IRB);
    Changed = true;
    return Changed;
  }
  if (kspecem_functions.isIn(F, "exception")) {
    BasicBlock *bb = &F.getEntryBlock();
    Instruction *inst = bb->getFirstNonPHI();

    IRBuilder<> IRB(*Ctx);
    IRB.SetInsertPoint(inst);
    KSpecEmFunction LF(*this, &F);
    Value *inSpec = LF.insertExceptionInterruptHook(IRB);

    /* do not handle the exception and return early if currently in speculation */
    BasicBlock *newBB = llvm::SplitBlock(bb, inst);
    bb->getTerminator()->eraseFromParent();

    BasicBlock *returnBB = BasicBlock::Create(*Ctx, "returnBlock", &F);
    IRBuilder<> retIRB(*Ctx);
    retIRB.SetInsertPoint(returnBB);
    retIRB.CreateRetVoid();

    IRBuilder<> condIRB(*Ctx);
    condIRB.SetInsertPoint(bb);
    Value *cond = condIRB.CreateIsNull(inSpec);
    condIRB.CreateCondBr(cond,
        newBB, returnBB);

    Changed = true;
    return Changed;
  }

  KSpecEmFunction LF(*this, &F);
  KSpecEmPassVisitor(LF).visit(F);
  Changed = true;

  return Changed;
}

Function *KSpecEmFunction::getCalledFunction(CallInst &CI,
    int *return_type, Value **calledValue) {
  Function *calledFunction = CI.getCalledFunction();
  if (calledFunction) {
    *return_type = 1;
    return calledFunction;
  } else {
    Value *calledVal = CI.getCalledOperand();
    if (calledVal && !isa<InlineAsm>(calledVal)) {
      *return_type = 2;

      if (isa<BitCastOperator>(calledVal)) {
        Value *operand = cast<BitCastOperator>(calledVal)->getOperand(0);
        if (isa<Function>(operand)) {
          Function *operandFunc = cast<Function>(operand);
          return operandFunc;
        }
      }

      *calledValue = calledVal;
      return NULL;
    } else {
      InlineAsm *inAsm = cast<InlineAsm>(calledVal);
      std::string asmString = inAsm->getAsmString();
      if (asmString.length() > 0) {
        // check if ~{memory} is in constraints
        InlineAsm::ConstraintInfoVector iV = inAsm->ParseConstraints();
        int hasMemoryGlobber = 0;
        for (int i = 0; i < iV.size(); i++) {
          InlineAsm::ConstraintCodeVector codes = iV[i].Codes;
          for (int j = 0; j < codes.size(); j++) {
            if (codes[j].compare("{memory}") == 0 ||
                codes[j].compare("m") == 0) {
              hasMemoryGlobber = 1;
              break;
            }
          }
        }
        /* whitelist a range of inlineasm strings that is deemed to be 'safe' */
        if (asmString.rfind("# __raw_save_flags\n\tpushf ; pop $0", 0) == 0 ||
            asmString.rfind("#KSPECEM_NO_RESTART") != std::string::npos ||
            asmString.compare("push $0 ; popf") == 0 ||
            asmString.compare("cli") == 0 ||
            asmString.compare("sti") == 0 ||
            asmString.compare("str $0") == 0 ||
            asmString.compare("rep; nop") == 0 ||
            (asmString.compare("mull \%edx") == 0 && !hasMemoryGlobber) ||
            (asmString.compare("bswapq $0") == 0 && !hasMemoryGlobber) ||
            (asmString.compare("rep; bsf $1,$0") == 0 && !hasMemoryGlobber) ||
            (asmString.compare("movl \%cs,$0") == 0 && !hasMemoryGlobber) ||
            (asmString.compare("movl \%ds,$0") == 0 && !hasMemoryGlobber) ||
            (asmString.compare("movl \%es,$0") == 0 && !hasMemoryGlobber) ||
            (asmString.compare("movl \%fs,$0") == 0 && !hasMemoryGlobber) ||
            (asmString.compare("movl \%gs,$0") == 0 && !hasMemoryGlobber) ||
            (asmString.rfind("mov $0, \%db", 0) == 0 && !hasMemoryGlobber) ||
            (asmString.rfind("mov \%db", 0) == 0 && !hasMemoryGlobber) ||
            (asmString.rfind("1:\09.byte 0x0f, 0x0b\0A.pushsection __bug_table,\22aw\22", 0) == 0 && !hasMemoryGlobber) ||
            (asmString.rfind("${0:c}:\0A\09.pushsection .discard.reachable\0A\09.long ${0:c}b", 0) == 0 && !hasMemoryGlobber) ||
            (asmString.compare("movq ${1:P},$0") == 0 && !hasMemoryGlobber) ||
            (asmString.compare("movq $1,$0") == 0 && !hasMemoryGlobber) ||
            (asmString.compare("decl $0") == 0 && !hasMemoryGlobber) ||
            (asmString.compare("incl $0") == 0 && !hasMemoryGlobber) ||
            (asmString.compare("incq $0") == 0 && !hasMemoryGlobber) ||
            (asmString.compare("bswapl $0") == 0 && !hasMemoryGlobber) ||
            (asmString.compare("orl $1,$0") == 0 && !hasMemoryGlobber) ||
            (asmString.compare("andb $1,$0") == 0 && !hasMemoryGlobber) ||
            (asmString.compare("addq $1, $0") == 0 && !hasMemoryGlobber) ||
            (asmString.compare("mov $0,\%cr3") == 0 && !hasMemoryGlobber) ||
            (asmString.compare("mov \%cr4,$0") == 0 && !hasMemoryGlobber) ||
            (asmString.compare("mov \%cr8,$0") == 0 && !hasMemoryGlobber) ||
            (asmString.compare("mov \%cr3,$0") == 0 && !hasMemoryGlobber)
          ) {
          return NULL;
        }

        *return_type = 3;
        return NULL;
      }
    }
  }
  return NULL;
}

void KSpecEmFunction::insertTimerInterruptHook(IRBuilder<> &IRB) {
    Argument *arg = nullptr;
    Function::arg_iterator I = F->arg_begin();
    if (I != F->arg_end())
      arg = &*I;

    std::vector<Value*> frameAddressArgs(1);
    frameAddressArgs[0] = IRB.getInt32(0);
    Function *fun = Intrinsic::getDeclaration(L.Mod, Intrinsic::frameaddress,
        IRB.getInt8PtrTy(L.Mod->getDataLayout().getAllocaAddrSpace()));
    CallInst *frameAddress = IRB.CreateCall(fun, frameAddressArgs);

    std::vector<Value*> interruptArgs(2);
    interruptArgs[0] = IRB.CreateBitCast(arg,
        PointerType::get(IntegerType::get(*L.Ctx, 8), 0));
    interruptArgs[1] = frameAddress;
    IRB.CreateCall(L.timerInterruptHook, interruptArgs);
}

Value *KSpecEmFunction::insertExceptionInterruptHook(IRBuilder<> &IRB) {
    Argument *arg = nullptr;
    Function::arg_iterator I = F->arg_begin();
    if (I != F->arg_end())
      arg = &*I;

    std::vector<Value*> frameAddressArgs(1);
    frameAddressArgs[0] = IRB.getInt32(0);
    Function *fun = Intrinsic::getDeclaration(L.Mod, Intrinsic::frameaddress,
        IRB.getInt8PtrTy(L.Mod->getDataLayout().getAllocaAddrSpace()));
    CallInst *frameAddress = IRB.CreateCall(fun, frameAddressArgs);

    std::vector<Value*> interruptArgs(3);
    interruptArgs[0] = IRB.CreateBitCast(arg,
        PointerType::get(IntegerType::get(*L.Ctx, 8), 0));
    interruptArgs[1] = frameAddress;
    interruptArgs[2] = IRB.getInt32(0);
    if (F->getName().equals("exc_page_fault")) {
      interruptArgs[2] = IRB.getInt32(1);
    }
    Value *inSpec = IRB.CreateCall(L.exceptionInterruptHook, interruptArgs);
    return inSpec;
}

void KSpecEmFunction::insertRestartHook(IRBuilder<> &IRB, int restart_type,
    Value *calledFunc) {
  std::vector<Value*> restartArgs(3);
  restartArgs[0] = llvm::ConstantPointerNull::get(llvm::Type::getInt8PtrTy(*L.Ctx));
  restartArgs[1] = IRB.getInt32(restart_type);
  if (calledFunc) {
    restartArgs[2] = IRB.CreatePointerCast(calledFunc, llvm::Type::getInt8PtrTy(*L.Ctx));
  } else {
    restartArgs[2] = llvm::ConstantPointerNull::get(llvm::Type::getInt8PtrTy(*L.Ctx));
  }
  IRB.CreateCall(L.restartHook, restartArgs);
}

void KSpecEmPassVisitor::visitCallInst(CallInst &CI) {
  if (LF.L.kspecem_functions.isIn(*LF.F, "nospec")) {
    return;
  }

  int restart_type = -1;
  Value *calledVal = NULL;
  Function *calledFunction = LF.getCalledFunction(CI, &restart_type, &calledVal);
  if (restart_type != -1 && !calledFunction) {
    IRBuilder<> IRB(*LF.L.Ctx);
    IRB.SetInsertPoint(&CI);
    LF.insertRestartHook(IRB, restart_type, calledVal);
  }
}

void KSpecEmPassVisitor::visitReturnInst(ReturnInst &RI) {
  if (LF.L.kspecem_functions.isIn(*LF.F, "nospec")) {
    return;
  }

  IRBuilder<> IRB(*LF.L.Ctx);
  IRB.SetInsertPoint(&RI);
  std::vector<Value*> returnAddressArgs(1);
  returnAddressArgs[0] = IRB.getInt32(0);
  Function *fun = Intrinsic::getDeclaration(LF.L.Mod, Intrinsic::returnaddress);
  CallInst *returnAddress = IRB.CreateCall(fun, returnAddressArgs);

  LF.insertRestartHook(IRB, 0, returnAddress);
}

void KSpecEmPassVisitor::visitStoreInst(StoreInst &SI) {
  if (LF.F->getName().startswith("kspecem_"))
    return;

	std::vector<Value*> args(1);

	/* the signature of the storeinsthook is (ptr) */
	args[0] = new BitCastInst(SI.getPointerOperand(), Type::getInt8PtrTy(*LF.L.Ctx), "", &SI);

  // TODO: replace with IRBuilder
  CallInst *callInst = CallInst::Create(LF.L.storeInstHook,
      args, "", &SI);
	callInst->setCallingConv(CallingConv::Fast);
	callInst->setIsNoInline();
}

void KSpecEmPassVisitor::visitMemIntrinsic(MemIntrinsic &MI) {
  if (LF.F->getName().startswith("kspecem_"))
    return;

  std::vector<Value*> args(2);

	args[0] = new BitCastInst(MI.getDest(), Type::getInt8PtrTy(*LF.L.Ctx), "", &MI);
	args[1] = MI.getLength();

  IRBuilder<> IRB(*LF.L.Ctx);
  IRB.SetInsertPoint(&MI);

  const DataLayout *DL  = &LF.L.Mod->getDataLayout();

	/* sometimes we get a i64 for the size */
	if (DL->getPointerSizeInBits() == 32) {
		if (static_cast<const IntegerType*>(args[1]->getType())->getBitWidth() != 32)  {
      args[1] = IRB.CreateTrunc(args[1], Type::getInt32Ty(*LF.L.Ctx), "");
		}
	} else {
		if (static_cast<const IntegerType*>(args[1]->getType())->getBitWidth() > 64)  {
      args[1] = IRB.CreateTrunc(args[1], Type::getInt64Ty(*LF.L.Ctx), "");
		} else if (static_cast<const IntegerType*>(args[1]->getType())->getBitWidth() < 64) {
      args[1] = IRB.CreateIntCast(args[1], Type::getInt64Ty(*LF.L.Ctx), false, "");
    }
	}

  CallInst *callInst = IRB.CreateCall(LF.L.memcpyHook, args);
  callInst->setCallingConv(CallingConv::Fast);
  callInst->setIsNoInline();
}

bool KSpecEm::runImpl(Module &M) {
  if (M.getName().contains("libkspecem.a"))
    return false;
  if (!init(M))
    return false;

  bool Changed = false;
  for (Function &F : M)
    Changed |= visitor(F);
  return true;
}

// New PM implementation
struct KSpecEmPass : PassInfoMixin<KSpecEmPass> {
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &) {
    bool Changed = KSpecEm().runImpl(M);
    return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
  }
};

// Legacy PM implementation
struct LegacyKSpecEmPass : public ModulePass {
  static char ID;
  LegacyKSpecEmPass() : ModulePass(ID) {}
  // Main entry point - the name conveys what unit of IR this is to be run on.
  bool runOnModule(Module &M) override {
    return KSpecEm().runImpl(M);
  }
};
} // namespace

//-----------------------------------------------------------------------------
// New PM Registration
//-----------------------------------------------------------------------------
llvm::PassPluginLibraryInfo getKSpecEmPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "KSpecEm", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerOptimizerLastEPCallback(
                [](llvm::ModulePassManager &PM,
                  llvm::PassBuilder::OptimizationLevel Level) {
                PM.addPass(KSpecEmPass());
                });
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "kspecem") {
                    MPM.addPass(KSpecEmPass());
                    return true;
                  }
                  return false;
                });
          }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getKSpecEmPassPluginInfo();
}

//-----------------------------------------------------------------------------
// Legacy PM Registration
//-----------------------------------------------------------------------------
// The address of this variable is used to uniquely identify the pass. The
// actual value doesn't matter.
char LegacyKSpecEmPass::ID = 0;

static RegisterPass<LegacyKSpecEmPass>
    X("legacy-kspecem", "KSpecEm Pass",
      false, // This pass DOES modify the CFG => false
      false // This pass is not a pure analysis pass => false
    );

static llvm::RegisterStandardPasses RegisterKSpecEmLTOThinPass(
    llvm::PassManagerBuilder::EP_OptimizerLast,
    [](const llvm::PassManagerBuilder &Builder,
       llvm::legacy::PassManagerBase &PM) { PM.add(new LegacyKSpecEmPass()); });

static llvm::RegisterStandardPasses RegisterKSpecEmLTOPass(
    llvm::PassManagerBuilder::EP_FullLinkTimeOptimizationLast,
    [](const llvm::PassManagerBuilder &Builder,
       llvm::legacy::PassManagerBase &PM) { PM.add(new LegacyKSpecEmPass()); });
