#include "InlineSimpleFunction.h"

using namespace discover;
using namespace llvm;

char InlineSimpleFunction::ID = 0;

bool hasGlobalValue(Function &F) {
  for (BasicBlock &B : F)
    for (Instruction &I : B)
      for (Value *Op : I.operands())
        if (GlobalValue* G = dyn_cast<GlobalValue>(Op))
          return true;

  return false;
}


// A function is called a transfer-call function if it only performs
// bitcast, call another function once and return the result;
bool isCallTransferFunc(Function &F) {
  debug() << "Checking Call-Transfer function: " << F.getName() << "\n";
  BasicBlockList &blockList = F.getBasicBlockList();
  if (blockList.size() == 1) {
    debug() << " has 1 block\n";
    BasicBlock &B = blockList.front();
    for (Instruction &I : B) {
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
bool isGEPTransferFunc(Function &F) {
  debug() << "Checking GEP-Transfer function: " << F.getName() << "\n";
  BasicBlockList &blockList = F.getBasicBlockList();
  if (blockList.size() == 1) {
    debug() << " has 1 block\n";
    BasicBlock &B = blockList.front();
    for (Instruction &I : B) {
      if (!(isa<GetElementPtrInst>(&I)) &&
          !(isa<BitCastInst>(&I)) &&
          !(isa<ReturnInst>(&I)))
        return false;
    }
    return true;
  }
  return false;
}

Function* InlineSimpleFunction::findCandidate(Module &M,
                                              FunctionSet processedFuncs) {
  FunctionList &funcList = M.getFunctionList();

  for (Function &F : funcList) {
    debug() << "Checking function: " << F.getName() << "\n";
    debug() << " is declaration? " << F.isDeclaration() << "\n";

    // if (F.isDeclaration() || )
    //   continue;

    bool procesed = false;
    for (Function *f: processedFuncs)
      if (f->getName().equals(F.getName())) {
        procesed = true;
        break;
      }

    if (procesed || isDiscoverTestingFunc(F))
      continue;

    if (!(F.hasLinkOnceLinkage()) && !(F.hasLinkOnceODRLinkage()) &&
        !(F.hasInternalLinkage()))
      continue;

    if (isCallTransferFunc(F) || isGEPTransferFunc(F)) {
      return &F;
    }
  }

  return NULL;
}

void InlineSimpleFunction::inlineFunction(Module &M, Function* F) {
  debug() << "* Start to inline function: " << F->getName() << "\n";
  StringRef funcName = F->getName();
  llvm::InlineFunctionInfo IFI;

  SmallSetVector<CallSite, 16> Calls;
  for (User *U : F->users())
    if (auto CS = CallSite(U))
      if (CS.getCalledFunction() == F)
        Calls.insert(CS);

  bool successful = true;

  for (CallSite CS : Calls) {
    InlineResult res = llvm::InlineFunction(CS, IFI);
    successful = successful && res;
  }

  if (successful) {
    debug() << "    Inline succeeded!\n";
    if (F->getNumUses() == 0) {
      F->eraseFromParent();
      debug() << "    Removed from parent!\n";
    }
  }
  else debug() << "    Inline failed!\n";
}

bool InlineSimpleFunction::runOnModule(Module &M) {
  FunctionSet processedFuncs = FunctionSet();

  while (true) {
    Function *F = findCandidate(M, processedFuncs);

    if (!F)
      break;

    inlineFunction(M, F);
    processedFuncs.insert(F);
  }

  return true;
}

bool InlineSimpleFunction::normalizeModule(Module &M) {
  debug() << "\n=========================================\n"
          << "Inlining Internal Functions...\n";

  InlineSimpleFunction pass;
  return pass.runOnModule(M);
}

static RegisterPass<InlineSimpleFunction> X("InlineSimpleFunction",
                                      "Normalize ConstantExpr",
                                      false /* Only looks at CFG */,
                                      false /* Analysis Pass */);

static RegisterStandardPasses Y(PassManagerBuilder::EP_EarlyAsPossible,
                                [](const PassManagerBuilder &Builder,
                                   legacy::PassManagerBase &PM) {
                                  PM.add(new InlineSimpleFunction());
                                });
