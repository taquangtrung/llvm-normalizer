#include "ElimStoreLoadAlloca.h"

using namespace discover;
using namespace llvm;

char ElimStoreLoadAlloca::ID = 0;

bool ElimStoreLoadAlloca::processStore(IRBuilder<> builder, StoreInst *instr) {
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

  return false;
}

bool ElimStoreLoadAlloca::processFunction(Function *func) {
  debug() << "\n** Processing function: " << func->getName() << "\n";
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

        if (StoreInst* storeInstr = dyn_cast<StoreInst>(instr)) {
          if (processStore(builder, storeInstr)) {
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

void ElimStoreLoadAlloca::handleFunctions(Module &M) {
  FunctionList &funcList = M.getFunctionList();

  for (auto it = funcList.begin(); it != funcList.end(); ++it) {
    Function *func = &(*it);
    bool continueProcess = true;
    while (continueProcess) {
      continueProcess = processFunction(func);
    }
  }
}

bool ElimStoreLoadAlloca::runOnModule(Module &M) {
  handleFunctions(M);
  return true;
}

bool ElimStoreLoadAlloca::normalizeModule(Module &M) {
  debug() << "\n=========================================\n"
          << "Eliminate Store-Load of Alloca...\n";

  ElimStoreLoadAlloca pass;
  return pass.runOnModule(M);
}

static RegisterPass<ElimStoreLoadAlloca> X("ElimStoreLoadAlloca",
                                 "ElimStoreLoadAlloca",
                                 false /* Only looks at CFG */,
                                 false /* Analysis Pass */);

static RegisterStandardPasses Y(PassManagerBuilder::EP_EarlyAsPossible,
                                [](const PassManagerBuilder &Builder,
                                   legacy::PassManagerBase &PM) {
                                  PM.add(new ElimStoreLoadAlloca());
                                });
