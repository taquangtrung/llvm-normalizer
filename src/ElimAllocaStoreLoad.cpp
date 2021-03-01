#include "ElimAllocaStoreLoad.h"

using namespace discover;
using namespace llvm;

/*
 * This pass eliminates a sequence of  Alloca, Load, Store instructions
 * that only serves as temporary data holder:
 *
 * For example:
 *     u = alloca int*
 *     store i, u
 *     x = load b
 *
 * Then the above instructions can be removed, and every appearance of `x`
 * can be replaced by `i`
 */


char ElimAllocaStoreLoad::ID = 0;

using AllocaStoreLoad = std::tuple<AllocaInst*, StoreInst*, LoadInst*>;

bool processStore(IRBuilder<> builder, StoreInst *instr) {
  Value* storeDst = instr->getOperand(1);

  if (AllocaInst* allocaInstr = dyn_cast<AllocaInst>(storeDst)) {

    int numStoreUsingAlloca = 0;
    for (User* u : allocaInstr->users()) {
      User* user = u;
      if (isa<StoreInst>(user)) {
        numStoreUsingAlloca++;
      }
    }

    if (numStoreUsingAlloca != 1)
      return false;

    bool isAllocaUsedOnlyInStoreLoad = true;
    for (User* u : allocaInstr->users()) {
      User* user = u;
      if (!isa<LoadInst>(user) && !isa<StoreInst>(user)) {
        isAllocaUsedOnlyInStoreLoad = false;
        break;
      }
    }

    if (!isAllocaUsedOnlyInStoreLoad)
      return false;

    Value* storeSrc = instr->getOperand(0);
    Function* func = instr->getFunction();

    debug() << "Processing Store Instr:\n" << *instr << "\n";

    for (User* u : allocaInstr->users()) {
      User* user = u;
      if (LoadInst* loadInstr = dyn_cast<LoadInst>(user)) {
        debug() << "\n   replace: " << *loadInstr << " -- by: " << *storeSrc << "\n";
        llvm::replaceOperand(func, loadInstr, storeSrc);
        loadInstr->removeFromParent();
      }
    }


    instr->removeFromParent();
    allocaInstr->removeFromParent();

    return true;
  }

  return false;
}

std::vector<AllocaStoreLoad> findRemovableAllocaStoreLoad(Function &F) {
  debug() << "\n** Processing function: " << F.getName() << "\n";
  std::vector<AllocaStoreLoad> candidateAllocaStoreLoadList;

  BasicBlockList &BS = F.getBasicBlockList();

  for (BasicBlock &B: BS) {
    for (Instruction &I: B) {
      if (!isa<AllocaInst>(&I))
        continue;

      AllocaInst *allocInst = dyn_cast<AllocaInst>(&I);

      if (allocInst->getNumUses() != 2)
        continue;

    }


    // BasicBlock *blk = &(*it);
    // IRBuilder<> builder(blk);

    // bool blockUpdated = true;

    // while (blockUpdated) {
    //   blockUpdated = false;

    //   for (auto it2 = blk->begin(); it2 != blk->end(); ++it2) {
    //     Instruction *instr = &(*it2);

    //     if (StoreInst* storeInstr = dyn_cast<StoreInst>(instr)) {
    //       if (processStore(builder, storeInstr)) {
    //         blockUpdated = true;
    //         funcUpdated = true;
    //         break;
    //       }
    //     }
    //   }
    // }
  }

  return candidateAllocaStoreLoadList;
}

bool ElimAllocaStoreLoad::runOnFunction(Function &F) {
  return true;
}

bool ElimAllocaStoreLoad::normalizeFunction(Function &F) {
  debug() << "\n=========================================\n"
          << "Eliminate Store-Load of Alloca in function: "
          << F.getName() << "\n";

  ElimAllocaStoreLoad pass;
  return pass.runOnFunction(F);
}

static RegisterPass<ElimAllocaStoreLoad> X("ElimAllocaStoreLoad",
                                           "ElimAllocaStoreLoad Pass",
                                           false /* Only looks at CFG */,
                                           false /* Analysis Pass */);

static RegisterStandardPasses Y(PassManagerBuilder::EP_EarlyAsPossible,
                                [](const PassManagerBuilder &Builder,
                                   legacy::PassManagerBase &PM) {
                                  PM.add(new ElimAllocaStoreLoad());
                                });
