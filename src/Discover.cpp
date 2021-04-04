#include "Discover.h"

// using namespace discover;
using namespace llvm;

/*
 * This pass run all transformation pass of discover
 */

char Discover::ID = 0;

/*
 * Entry function for this FunctionPass, can be used by llvm-opt
 */
// PreservedAnalyses Discover::run(Function &F, FunctionAnalysisManager &FAM) {
bool Discover::runOnModule(Module &M) {
  DominatorTree &DT = getAnalysis<DominatorTreeWrapperPass>(F).getDomTree();
  // DominatorTree &DT = FAM.getResult<DominatorTreeAnalysis>(F);

  // find and eliminate identical CastInst
  IdentInstsList identCastList = findCastInstsOfSameSourceAndType(F);
  eliminateIdenticalInstrs(F, DT, identCastList);

  // find and eliminate identical PHINode
  IdentInstsList identPHIList = findPHINodeOfSameIncoming(F);
  eliminateIdenticalInstrs(F, DT, identPHIList);

  // find and eliminate identical GetElementPtrInst
  IdentInstsList identGEPList = findGEPOfSameElemPtr(F);
  eliminateIdenticalInstrs(F, DT, identGEPList);

  return true;
}

/*
 * Static function, used by this normalizer
 */
bool Discover::normalizeFunction(Function &F) {
  debug() << "\n=========================================\n"
          << "Eliminate Common Instruction in function: "
          << F.getName() << "\n";

  Discover pass;

  // FunctionAnalysisManager FAM;
  // return pass.run(F, FAM);

  return pass.runOnFunction(F);
}

static RegisterPass<Discover> X("Discover",
                                     "Discover",
                                     false /* Only looks at CFG */,
                                     false /* Analysis Pass */);

static RegisterStandardPasses Y(PassManagerBuilder::EP_EarlyAsPossible,
                                [](const PassManagerBuilder &Builder,
                                   legacy::PassManagerBase &PM) {
                                  PM.add(new Discover());
                                });
