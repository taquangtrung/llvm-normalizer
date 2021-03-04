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

using AllocaStoreLoads = std::tuple<AllocaInst*, StoreInst*, std::vector<LoadInst*>>;

void removeAllocaStoreLoad(Function &F,
                           std::vector<AllocaStoreLoads> allocaStoreLoadList) {
  for (auto it = allocaStoreLoadList.begin(); it != allocaStoreLoadList.end(); it++) {
    AllocaStoreLoads instrTuple = *it;

    AllocaInst* allocInst = std::get<0>(instrTuple);
    StoreInst* storeInst = std::get<1>(instrTuple);
    std::vector<LoadInst*> loadInsts = std::get<2>(instrTuple);

    // debug() << "- Elim AllocaInst: " << *allocInst << "\n";

    Value* storeSrc = storeInst->getOperand(0);

    for (auto it = loadInsts.begin(); it != loadInsts.end(); it++) {
      LoadInst* loadInst = *it;
      // debug() << "   replace: " << *loadInst << "\n"
      //         << "      by: " << *storeSrc << "\n";
      llvm::replaceOperand(&F, loadInst, storeSrc);
      loadInst->removeFromParent();
    }

    storeInst->removeFromParent();
    allocInst->removeFromParent();
  }
}

std::vector<AllocaStoreLoads> findRemovableAllocaStoreLoad(Function &F) {
  std::vector<AllocaStoreLoads> candidateAllocaStoreLoadList;

  BasicBlockList &BS = F.getBasicBlockList();

  for (BasicBlock &B: BS) {
    for (Instruction &I: B) {
      if (!isa<AllocaInst>(&I))
        continue;

      AllocaInst *allocInst = dyn_cast<AllocaInst>(&I);
      // debug() << "AllocaInst: " << *allocInst << "\n";

      StoreInst *storeInst;
      std::vector<LoadInst*> loadInsts;

      bool hasOnlyStoreDstLoad = true;
      int numStoreInst = 0;
      int numLoadInst = 0;

      for (auto it = allocInst->user_begin(); it != allocInst->user_end(); it++) {
        Value *instUser = *it;
        // debug() << "  user: " << *it->getUser() << "\n";

        if (storeInst = dyn_cast<StoreInst>(instUser)) {
          if (storeInst->getOperand(1) != allocInst)
            hasOnlyStoreDstLoad = false;
          // debug() << "  StoreInst: " << *storeInst << "\n";
          numStoreInst++;
        }
        else if (LoadInst *loadInst = dyn_cast<LoadInst>(instUser)) {
          // debug() << "  LoadInst: " << *loadInst << "\n";
          loadInsts.push_back(loadInst);
          numLoadInst++;
        }
        else {
          hasOnlyStoreDstLoad = false;
          break;
        }
      }

      if (hasOnlyStoreDstLoad && numStoreInst == 1 && numLoadInst > 0) {
        AllocaStoreLoads candidate = std::make_tuple(allocInst, storeInst, loadInsts);
        candidateAllocaStoreLoadList.push_back(candidate);
      }
    }
  }

  return candidateAllocaStoreLoadList;
}

bool ElimAllocaStoreLoad::runOnFunction(Function &F) {
  std::vector<AllocaStoreLoads> instrTupleList = findRemovableAllocaStoreLoad(F);
  removeAllocaStoreLoad(F, instrTupleList);
  return true;
}

bool ElimAllocaStoreLoad::normalizeFunction(Function &F) {
  debug() << "\n=========================================\n"
          << "Eliminate AllocaStoreLoads in function: "
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
