#include "InlineInternalFunction.h"

using namespace discover;
using namespace llvm;

char InlineInternalFunction::ID = 0;

bool hasGlobalValue(Function &F) {
  for (BasicBlock &BB : F)
    for (Instruction &I : BB)
      for (Value *Op : I.operands())
        if (GlobalValue* G = dyn_cast<GlobalValue>(Op))
          return true;

  return false;
}


// A function is called a transfer-call function if it only performs
// bitcast, call another function once and return the result;
bool isFuncTransferCall(Function *F) {
  debug() << "Checking direct-transfer-call function: " << F->getName() << "\n";
  BasicBlockList &blockList = F->getBasicBlockList();
  if (blockList.size() == 1) {
    debug() << " has 1 block\n";
    BasicBlock &BB = blockList.front();
    for (Instruction &I : BB) {
      if (!(isa<CallInst>(&I)) &&
          !(isa<BitCastInst>(&I)) &&
          !(isa<ReturnInst>(&I)))
        return false;
    }
    return true;
  }
  return false;
}

// A function is called a transfer-call function if it only performs
// bitcast, call another function once and return the result;
bool isFuncTransferGEP(Function *F) {
  debug() << "Checking direct-transfer-call function: " << F->getName() << "\n";
  BasicBlockList &blockList = F->getBasicBlockList();
  if (blockList.size() == 1) {
    debug() << " has 1 block\n";
    BasicBlock &BB = blockList.front();
    for (Instruction &I : BB) {
      if (!(isa<GetElementPtrInst>(&I)) &&
          !(isa<BitCastInst>(&I)) &&
          !(isa<ReturnInst>(&I)))
        return false;
    }
    return true;
  }
  return false;
}

Function* InlineInternalFunction::findCandidate(Module &M,
                                                FunctionSet processedFuncs) {
  FunctionList &funcList = M.getFunctionList();

  for (auto it = funcList.begin(); it != funcList.end(); ++it) {
    Function *func = &(*it);

    bool visited = false;
    for (Function *f: processedFuncs)
      if (f->getName().equals(func->getName())) {
        visited = true;
        break;
      }

    if (visited || isTestingFunc(func))
      continue;

    // if (isFuncTransferCall(func))
    //   return func;

    if (!(func->hasLinkOnceLinkage()) &&
        !(func->hasLinkOnceODRLinkage()) &&
        !(func->hasInternalLinkage()))
      continue;

    if (isFuncTransferCall(func) ||
        isFuncTransferGEP(func))
      return func;
  }

  return NULL;
}

void InlineInternalFunction::inlineFunction(Module &M, Function* func) {
  debug() << "* Start to inline function: " << func->getName() << "\n";
  StringRef funcName = func->getName();
  llvm::InlineFunctionInfo IFI;

  SmallSetVector<CallSite, 16> Calls;
  for (User *U : func->users())
    if (auto CS = CallSite(U))
      if (CS.getCalledFunction() == func)
        Calls.insert(CS);

  bool successful = true;

  for (CallSite CS : Calls) {
    InlineResult res = InlineFunction(CS, IFI);
    successful = successful && res;
  }

  if (successful) {
    debug() << "    Inline succeeded!\n";
    if (func->getNumUses() == 0) {
      func->eraseFromParent();
      debug() << "    Removed from parent!\n";
    }
  }
  else debug() << "    Inline failed!\n";
}

bool InlineInternalFunction::runOnModule(Module &M) {
  FunctionSet processedFuncs = FunctionSet();

  while (true) {
    Function *func = findCandidate(M, processedFuncs);

    if (!func)
      break;

    inlineFunction(M, func);
    processedFuncs.insert(func);
  }


  std::string str;
  llvm::raw_string_ostream rso(str);
  if (verifyModule(M, &rso)) {
    debug() << "After inlining functions\n";
    debug() << "Module is broken: " << rso.str() << "\n";
    debug() << M;
  }



  return true;

}

bool InlineInternalFunction::normalizeModule(Module &M) {
  debug() << "\n=========================================\n"
          << "Inlining Internal Functions...\n";

  InlineInternalFunction pass;
  return pass.runOnModule(M);
}

static RegisterPass<InlineInternalFunction> X("InlineInternalFunction",
                                      "Normalize ConstantExpr",
                                      false /* Only looks at CFG */,
                                      false /* Analysis Pass */);

static RegisterStandardPasses Y(PassManagerBuilder::EP_EarlyAsPossible,
                                [](const PassManagerBuilder &Builder,
                                   legacy::PassManagerBase &PM) {
                                  PM.add(new InlineInternalFunction());
                                });
