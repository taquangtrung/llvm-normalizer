#include "ElimUnusedAuxFunction.h"

using namespace discover;
using namespace llvm;

char ElimUnusedAuxFunction::ID = 0;

bool ElimUnusedAuxFunction::runOnModule(Module &M) {
  StringRef passName = this->getPassName();
  debug() << "=========================================\n"
          << "Running Module Pass: " << passName << "\n";

  FunctionList &funcList = M.getFunctionList();

  FunctionSet unusedFuncs;

  for (auto it = funcList.begin(); it != funcList.end(); ++it) {
    Function *func = &(*it);

    StringRef funcName = func->getName();

    // not auxiliary function of Discover
    if (!funcName.contains("__assert") && !funcName.contains("__refute"))
      continue;

    if (func->getNumUses() == 0)
      unusedFuncs.insert(func);

    // outs() << "Function name: " << func->getName()
    //        << ", num of uses: " << func->getNumUses() << "\n";
  }

  for (Function* func: unusedFuncs) {
    // outs() << "Remove function: " << func->getName() << "\n";

    func->removeFromParent();
    // func->deleteValue();
  }

  debug() << "Finish Module Pass: " << passName << "\n";

  return true;
}

bool ElimUnusedAuxFunction::normalizeModule(Module &M) {
  ElimUnusedAuxFunction pass;
  return pass.runOnModule(M);
}

static RegisterPass<ElimUnusedAuxFunction> X("ElimUnusedAuxFunction",
    "ElimUnusedAuxFunction",
    false /* Only looks at CFG */,
    false /* Analysis Pass */);

static RegisterStandardPasses Y(PassManagerBuilder::EP_EarlyAsPossible,
    [](const PassManagerBuilder &Builder, legacy::PassManagerBase &PM) {
      PM.add(new ElimUnusedAuxFunction());});
