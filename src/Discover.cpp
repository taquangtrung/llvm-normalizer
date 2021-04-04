#include "Discover.h"
#include "InitGlobal.h"
#include "UninlineInstruction.h"
#include "InlineSimpleFunction.h"
#include "ElimUnusedAuxFunction.h"
#include "ElimUnusedGlobal.h"
#include "ElimIdenticalInstrs.h"
#include "CombineGEP.h"
#include "ElimAllocaStoreLoad.h"

using namespace discover;
using namespace llvm;

/*
 * This pass run all transformation pass of discover
 */

char Discover::ID = 0;

/*
 * Entry function for this FunctionPass, can be used by llvm-opt
 */

void normalizeGlobal(Module& M) {
  InitGlobal::normalizeModule(M);
}


void normalizeFunction(ModulePass* MP, Function& F) {
  UninlineInstruction::normalizeFunction(F);
  CombineGEP::normalizeFunction(F);
  ElimIdenticalInstrs::normalizeFunction(F);
  ElimAllocaStoreLoad::normalizeFunction(F);
}


void normalizeModule(Module& M) {
  ElimUnusedAuxFunction::normalizeModule(M);
  InlineSimpleFunction::normalizeModule(M);
  ElimUnusedGlobal::normalizeModule(M);
}

void Discover::getAnalysisUsage(AnalysisUsage &AU) const {
  // AU.addRequired<DominatorTreeWrapperPass>();
  AU.setPreservesAll();
  AU.addRequired<llvm::AAResultsWrapperPass>();
}


// bool Discover::runOnFunction(Function &F) {
//   AliasAnalysis *AA = &this->getAnalysis<AAResultsWrapperPass>().getAAResults();

//   return false;
// }

// bool Discover::runOnModule(Module &M) {
//   FunctionList &FS = M.getFunctionList();

//   llvm::DominatorTree domTree{&func, func};
//   llvm::LoopInfo loopInfo{&func, domTree};

//   for (Module::iterator func_iter = M.begin(), func_iter_end = M.end();
//        func_iter != func_iter_end; ++func_iter) {
//     Function &F = *func_iter;
//     AliasAnalysis *AA = &getAnalysis<AAResultsWrapperPass>(F).getAAResults();
//   }

//   // for (Function &F: FS)


//   return false;
// }

bool Discover::runOnModule(Module &M) {
  // Normalize globals first
  normalizeGlobal(M);

  // Run each FunctionPass
  FunctionList &FS = M.getFunctionList();

  for (Function &F: FS) {
    normalizeFunction(this, F);
  }

  // Run ModulePass
  normalizeModule(M);
  return true;
}

static RegisterPass<Discover> XF("discover",
                                "Discover",
                                true /* Only looks at CFG */,
                                true /* Analysis Pass */);

// static RegisterStandardPasses Y(PassManagerBuilder::EP_EarlyAsPossible,
//                                 [](const PassManagerBuilder &Builder,
//                                    legacy::PassManagerBase &PM) {
//                                   PM.add(new Discover());
//                                 });
