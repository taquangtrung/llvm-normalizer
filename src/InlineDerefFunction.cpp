#include "InlineDerefFunction.h"

using namespace discover;
using namespace llvm;

char InlineDerefFunction::ID = 0;

bool hasGlobalValue(Function &F) {
  for (BasicBlock &BB : F)
    for (Instruction &I : BB)
      for (Value *Op : I.operands())
        if (GlobalValue* G = dyn_cast<GlobalValue>(Op))
          return true;

  return false;
}

Function* InlineDerefFunction::findCandidate(Module &M, FunctionList visited) {
  FunctionList &funcList = M.getFunctionList();

  for (auto it = funcList.begin(); it != funcList.end(); ++it) {
    Function *func = &(*it);

    if (visited.is_member(func))
      continue;


    int numParams = func->getNumOperands();

    GlobalValue::LinkageTypes linkage = func->getLinkage();

    AttributeList attributes = func->getAttributes();

    if ((func->hasLinkOnceLinkage() || func->hasLinkOnceODRLinkage()) &&
        !hasGlobalValue(*func))
      return func;

    if (func->getDereferenceableBytes(0) > 0)
      return func;
  }

  return NULL;
}

void InlineDerefFunction::inlineFunction(Module &M, Function* func) {
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

  // M.getFunctionList().erase(func);
  if (successful) {
    debug() << "    Inline succeeded!\n";
    func->eraseFromParent();
  }
  else
    debug() << "    Inline failed!\n";
}

bool InlineDerefFunction::runOnModule(Module &M) {

  FunctionList visited;
  while (true) {
    Function *func = findCandidate(M);
    // debug() << "Prepare to inline: " << func->getName() << "\n";

    if (!func)
      break;

    if (!inlineFunction(M, func))
      visited.addNodeToList(func);
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

bool InlineDerefFunction::normalizeModule(Module &M) {
  InlineDerefFunction pass;
  return pass.runOnModule(M);
}

static RegisterPass<InlineDerefFunction> X("InlineDerefFunction",
                                      "Normalize ConstantExpr",
                                      false /* Only looks at CFG */,
                                      false /* Analysis Pass */);

static RegisterStandardPasses Y(PassManagerBuilder::EP_EarlyAsPossible,
                                [](const PassManagerBuilder &Builder,
                                   legacy::PassManagerBase &PM) {
                                  PM.add(new InlineDerefFunction());
                                });
