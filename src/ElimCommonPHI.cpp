#include "ElimCommonPHI.h"

using namespace discover;
using namespace llvm;

char ElimCommonPHI::ID = 0;

PHINode* ElimCommonPHI::findPHINodeOfSameIncoming(PHINode *instr) {
  BasicBlock *blk = instr->getParent();

  int numIncoming = instr->getNumIncomingValues();

  for (auto it = blk->begin(); it != blk->end(); ++it) {
    if (PHINode* otherInstr = dyn_cast<PHINode>(&(*it))) {
      if (otherInstr == instr)
        continue;

      if (otherInstr->getNumIncomingValues() == numIncoming) {
        for (int i = 0; i < numIncoming; i++)
          if (otherInstr->getIncomingValue(i) != instr->getIncomingValue(i))
            return NULL;

        return otherInstr;
      }
    }
  }

  return NULL;
}

bool ElimCommonPHI::processFunction(Function *func) {
  BasicBlockList &blockList = func->getBasicBlockList();
  bool stop = true;

  for (auto it = blockList.begin(); it != blockList.end(); ++it) {
    BasicBlock *blk = &(*it);
    IRBuilder<> builder(blk);

    for (auto it2 = blk->begin(); it2 != blk->end(); ++it2) {
      Instruction *instr = &(*it2);

      if (PHINode* phiInstr = dyn_cast<PHINode>(instr)) {
        PHINode *otherInstr = findPHINodeOfSameIncoming(phiInstr);

        if (otherInstr != NULL) {
          llvm::replaceOperand(func, otherInstr, phiInstr);
          otherInstr->eraseFromParent();
          return true;
        }
      }
    }
  }

  return false;
}

/**
 * un-inline ConstExpr in Functions' instructions
 */
void ElimCommonPHI::handleFunctions(Module &M) {

  FunctionList &funcList = M.getFunctionList();

  for (auto it = funcList.begin(); it != funcList.end(); ++it) {
    Function *func = &(*it);
    bool continueProcess = true;
    while (continueProcess) {
      continueProcess = processFunction(func);
    }
  }
}

bool ElimCommonPHI::runOnModule(Module &M) {
  handleFunctions(M);

  return true;
}

bool ElimCommonPHI::normalizeModule(Module &M) {
  ElimCommonPHI pass;
  return pass.runOnModule(M);
}

static RegisterPass<ElimCommonPHI> X("ElimCommonPHI",
                                 "ElimCommonPHI",
                                 false /* Only looks at CFG */,
                                 false /* Analysis Pass */);

static RegisterStandardPasses Y(PassManagerBuilder::EP_EarlyAsPossible,
                                [](const PassManagerBuilder &Builder,
                                   legacy::PassManagerBase &PM) {
                                  PM.add(new ElimCommonPHI());
                                });
