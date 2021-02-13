#include "CombineGEP.h"

using namespace discover;
using namespace llvm;

char CombineGEP::ID = 0;

bool CombineGEP::processGEP(IRBuilder<> builder, GetElementPtrInst *instr) {
  debug() << "\nprocessing GEP instr:\n" << *instr << "\n";
  if (instr->getNumUses() == 1) {

    // TODO: need a better way to find the first user
    User *user;
    for (User* u : instr->users()) {
      user = u;
      break;
    }

    if (GetElementPtrInst* gepInstr = dyn_cast<GetElementPtrInst>(user)) {
      debug() << "\n  next GEP instr:\n" << *gepInstr << "\n";
      if (gepInstr->getPointerOperand() == instr) {
        if (ConstantInt *constantInt = dyn_cast<ConstantInt>(gepInstr->getOperand(1))) {

          if ((constantInt->getZExtValue() == 0) &&
              (instr->getNumUses() == 1) &&
              (gepInstr->getNumUses() == 1)) {
            std::vector<Value*> newIdxs;

            for (int i = 1; i < instr->getNumOperands(); i++)
              newIdxs.push_back(instr->getOperand(i));

            for (int i = 2; i < gepInstr->getNumOperands(); i++)
              newIdxs.push_back(gepInstr->getOperand(i));

            Value* ptr = instr->getPointerOperand();
            Instruction* newGepInstr = GetElementPtrInst::CreateInBounds(ptr, newIdxs);

            builder.SetInsertPoint(gepInstr);
            builder.Insert(newGepInstr);

            Function* func = instr->getFunction();
            llvm::replaceOperand(func, gepInstr, newGepInstr);

            instr->removeFromParent();
            gepInstr->removeFromParent();

            return true;
          }
        }
      }
    }
  }

  return false;
}

bool CombineGEP::processFunction(Function *func) {
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

        if (GetElementPtrInst* gepInstr = dyn_cast<GetElementPtrInst>(instr)) {
          if (processGEP(builder, gepInstr)) {
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

void CombineGEP::handleFunctions(Module &M) {
  FunctionList &funcList = M.getFunctionList();

  for (auto it = funcList.begin(); it != funcList.end(); ++it) {
    Function *func = &(*it);
    bool continueProcess = true;
    while (continueProcess) {
      continueProcess = processFunction(func);
    }
  }
}

bool CombineGEP::runOnModule(Module &M) {
  handleFunctions(M);
  return true;
}

bool CombineGEP::normalizeModule(Module &M) {
  debug() << "\n=========================================\n"
          << "Combining GEP...\n";

  CombineGEP pass;
  return pass.runOnModule(M);
}

static RegisterPass<CombineGEP> X("CombineGEP",
                                     "CombineGEP",
                                     false /* Only looks at CFG */,
                                     false /* Analysis Pass */);

static RegisterStandardPasses Y(PassManagerBuilder::EP_EarlyAsPossible,
                                [](const PassManagerBuilder &Builder,
                                   legacy::PassManagerBase &PM) {
                                  PM.add(new CombineGEP());
                                });
