#include "ElimUnusedGlobal.h"

using namespace discover;
using namespace llvm;

char ElimUnusedGlobal::ID = 0;

bool ElimUnusedGlobal::runOnModule(Module &M) {
  debug() << "=========================================\n"
          << "Eliminating Unused Global Variables...\n";

  for (Function &F: M) {
    DominatorTree &DT = getAnalysis<DominatorTreeWrapperPass>(F).getDomTree();
  }

  GlobalVariableList &globalList = M.getGlobalList();
  SmallSetVector<GlobalVariable*, 16> removableGlobals;

  for (GlobalVariable &global : globalList)
    if (global.getNumUses() == 0)
      removableGlobals.insert(&global);

  for (GlobalVariable *global : removableGlobals) {
    debug() << " - Deleting " << *global << "\n";
    global->removeFromParent();
  }

  return true;
}

bool ElimUnusedGlobal::normalizeModule(Module &M) {
  ElimUnusedGlobal pass;
  return pass.runOnModule(M);
}

static RegisterPass<ElimUnusedGlobal> X("ElimUnusedGlobal",
    "ElimUnusedGlobal",
    false /* Only looks at CFG */,
    true /* Analysis Pass */);

static RegisterStandardPasses Y(PassManagerBuilder::EP_EarlyAsPossible,
    [](const PassManagerBuilder &Builder, legacy::PassManagerBase &PM) {
      PM.add(new ElimUnusedGlobal());});
