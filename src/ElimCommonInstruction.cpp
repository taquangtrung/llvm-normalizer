#include "ElimCommonInstruction.h"

using namespace discover;
using namespace llvm;

char ElimCommonInstruction::ID = 0;

GetElementPtrInst* ElimCommonInstruction::findGEPOfSameElement(GetElementPtrInst *instr) {
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

PHINode* ElimCommonInstruction::findPHINodeOfSameIncoming(PHINode *instr) {
  BasicBlock *blk = instr->getParent();

  int numIncoming = instr->getNumIncomingValues();

  for (auto it = blk->begin(); it != blk->end(); ++it) {
    if (PHINode* otherInstr = dyn_cast<PHINode>(&(*it))) {
      if (otherInstr == instr)
        continue;

      if (otherInstr->getNumIncomingValues() != numIncoming)
        continue;

      bool hasSameIncomings = true;

      for (int i = 0; i < numIncoming; i++)
        if (otherInstr->getIncomingValue(i) != instr->getIncomingValue(i)) {
          hasSameIncomings = false;
        }

      if (hasSameIncomings)
        return otherInstr;
    }
  }

  return NULL;
}


CastInst* ElimCommonInstruction::findCastInstOfCommonSource(CastInst *instr) {
  BasicBlock *blk = instr->getParent();

  int numOperands = instr->getNumOperands();

  // debug() << "Find GEP of same element for:\n" << *instr << "\n";

  for (auto it = blk->begin(); it != blk->end(); ++it) {
    if (CastInst* otherInstr = dyn_cast<CastInst>(&(*it))) {
      if (otherInstr == instr)
        continue;

      if ((numOperands == otherInstr->getNumOperands()) &&
          (instr->getOperand(0) == otherInstr->getOperand(0)) &&
          (instr->getDestTy() == otherInstr->getDestTy()))
        return otherInstr;
    }
  }

  return NULL;
}

bool ElimCommonInstruction::processFunction(Function *func) {
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

        Instruction* otherInstr = NULL;

        if (GetElementPtrInst* gepInstr = dyn_cast<GetElementPtrInst>(instr))
          otherInstr = findGEPOfSameElement(gepInstr);
        else if (PHINode* phiInstr = dyn_cast<PHINode>(instr))
          otherInstr = findPHINodeOfSameIncoming(phiInstr);
        else if (CastInst* castInstr = dyn_cast<CastInst>(instr))
          otherInstr = findCastInstOfCommonSource(castInstr);

        if (otherInstr != NULL) {
          debug() << "Substitute:\n  " << *otherInstr << "\n"
                  << "  by:\n  " << *instr << "\n";
          llvm::replaceOperand(func, otherInstr, instr);
          otherInstr->eraseFromParent();
          blockUpdated = true;
          funcUpdated = true;
          break;
        }
      }
    }
  }

  return funcUpdated;
}

void ElimCommonInstruction::handleFunctions(Module &M) {
  FunctionList &funcList = M.getFunctionList();

  for (auto it = funcList.begin(); it != funcList.end(); ++it) {
    Function *func = &(*it);
    bool continueProcess = true;
    while (continueProcess) {
      continueProcess = processFunction(func);
    }
  }
}

bool ElimCommonInstruction::runOnModule(Module &M) {
  handleFunctions(M);
  return true;
}

bool ElimCommonInstruction::normalizeModule(Module &M) {
  debug() << "\n=========================================\n"
          << "Eliminating Common Instruction...\n";

  ElimCommonInstruction pass;
  return pass.runOnModule(M);
}

static RegisterPass<ElimCommonInstruction> X("ElimCommonInstruction",
                                     "ElimCommonInstruction",
                                     false /* Only looks at CFG */,
                                     false /* Analysis Pass */);

static RegisterStandardPasses Y(PassManagerBuilder::EP_EarlyAsPossible,
                                [](const PassManagerBuilder &Builder,
                                   legacy::PassManagerBase &PM) {
                                  PM.add(new ElimCommonInstruction());
                                });
