#include "ElimCommonGEP.h"

using namespace discover;
using namespace llvm;

char ElimCommonGEP::ID = 0;

GetElementPtrInst* ElimCommonGEP::findGEPOfSameElement(GetElementPtrInst *instr) {
  BasicBlock *blk = instr->getParent();

  int numOperands = instr->getNumOperands();

  // debug() << "Find GEP of same element for:\n" << *instr << "\n";

  for (auto it = blk->begin(); it != blk->end(); ++it) {
    if (GetElementPtrInst* otherInstr = dyn_cast<GetElementPtrInst>(&(*it))) {
      if (otherInstr == instr)
        continue;

      if (numOperands != otherInstr->getNumOperands())
        continue;

      bool hasSameOperands = true;

      // debug() << " - other instr:\n" << *otherInstr << "\n";

      for (int i = 0; i < numOperands; i++)
        if (instr->getOperand(i) != otherInstr->getOperand(i)) {
          // debug() << "     + different operand: " << *(instr->getOperand(i))
          //         << ", " << *(otherInstr->getOperand(i)) << "\n";
          hasSameOperands = false;
        }

      if (hasSameOperands)
        return otherInstr;
    }
  }

  return NULL;
}

bool ElimCommonGEP::processFunction(Function *func) {
  BasicBlockList &blockList = func->getBasicBlockList();
  bool stop = true;
  bool funcUpdated = false;

  for (auto it = blockList.begin(); it != blockList.end(); ++it) {
    BasicBlock *blk = &(*it);
    IRBuilder<> builder(blk);

    bool blockUpdated = true;

    while (blockUpdated) {
      blockUpdated = false;

      for (auto it2 = blk->begin(); it2 != blk->end(); ++it2) {
        Instruction *instr = &(*it2);

        if (GetElementPtrInst* gepInstr = dyn_cast<GetElementPtrInst>(instr)) {
          GetElementPtrInst *otherInstr = findGEPOfSameElement(gepInstr);

          if (otherInstr != NULL) {
            debug() << "Substitute:\n  " << *otherInstr << "\n"
                    << "  by:\n  " << *gepInstr << "\n";
            llvm::replaceOperand(func, otherInstr, gepInstr);
            otherInstr->eraseFromParent();
            blockUpdated = true;
            funcUpdated = true;
            break;
          }
        }
      }
    }
  }

  return funcUpdated;
}

void ElimCommonGEP::handleFunctions(Module &M) {
  FunctionList &funcList = M.getFunctionList();

  for (auto it = funcList.begin(); it != funcList.end(); ++it) {
    Function *func = &(*it);
    bool continueProcess = true;
    while (continueProcess) {
      continueProcess = processFunction(func);
    }
  }
}

bool ElimCommonGEP::runOnModule(Module &M) {
  handleFunctions(M);
  return true;
}

bool ElimCommonGEP::normalizeModule(Module &M) {
  debug() << "\n=========================================\n"
          << "Eliminating Common GEP...\n";

  ElimCommonGEP pass;
  return pass.runOnModule(M);
}

static RegisterPass<ElimCommonGEP> X("ElimCommonGEP",
                                     "ElimCommonGEP",
                                     false /* Only looks at CFG */,
                                     false /* Analysis Pass */);

static RegisterStandardPasses Y(PassManagerBuilder::EP_EarlyAsPossible,
                                [](const PassManagerBuilder &Builder,
                                   legacy::PassManagerBase &PM) {
                                  PM.add(new ElimCommonGEP());
                                });
