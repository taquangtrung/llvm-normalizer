#include "SplitCallInstrs.h"

using namespace discover;
using namespace llvm;

using GEPInstList = std::vector<GetElementPtrInst*>;

/*
 * This pass implement a callsplit pass, which is similar to LLVM,
 * but splitting is allowed on non-constant arguments of function calls.
 */

char SplitCallInstrs::ID = 0;


/*
 * Entry function for this FunctionPass, can be used by llvm-opt
 */
bool SplitCallInstrs::runOnFunction(Function &F) {
  std::vector<GEPInstList> allGEPList = findCombinableGEPList(F);
  combineGEPInstructions(F, allGEPList);
  return true;
}

/*
 * Static function, used by this normalizer
 */
bool SplitCallInstrs::normalizeFunction(Function &F) {
  debug() << "\n=========================================\n"
          << "Combining GEP Instruction in function: "
          << F.getName() << "\n";

  SplitCallInstrs pass;
  return pass.runOnFunction(F);
}

static RegisterPass<SplitCallInstrs> X("SplitCallInstrs",
                                       "SplitCallInstrs Pass",
                                       false /* Only looks at CFG */,
                                       false /* Analysis Pass */);

static RegisterStandardPasses Y(PassManagerBuilder::EP_EarlyAsPossible,
                                [](const PassManagerBuilder &Builder,
                                   legacy::PassManagerBase &PM) {
                                  PM.add(new SplitCallInstrs());
                                });
