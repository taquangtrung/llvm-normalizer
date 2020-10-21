#include "UnwrapGEP.h"

using namespace discover;
using namespace llvm;

char UnwrapGEP::ID = 0;


void UnwrapGEP::replaceOperand(Function *func, Value *replacee, Value *replacer) {
  BasicBlockListType &blockList = func->getBasicBlockList();

  for (auto it = blockList.begin(); it != blockList.end(); ++it) {
    BasicBlock *blk = &(*it);

    for (auto it2 = blk->begin(); it2 != blk->end(); ++it2) {
      Instruction *instr = &(*it2);

      for (int i = 0; i < instr->getNumOperands(); i++) {
        Value *operand = instr->getOperand(i);
        if (operand == replacee)
          instr->setOperand(i, replacer);
      }
    }
  }
}

bool UnwrapGEP::processFunction(Function *func) {
  BasicBlockListType &blockList = func->getBasicBlockList();
  bool stop = true;

  for (auto it = blockList.begin(); it != blockList.end(); ++it) {
    BasicBlock *blk = &(*it);
    IRBuilder<> builder(blk);

    for (auto it2 = blk->begin(); it2 != blk->end(); ++it2) {
      Instruction *instr = &(*it2);

      if (GetElementPtrInst* gepInstr = dyn_cast<GetElementPtrInst>(instr)) {
        if (gepInstr->getNumOperands() == 2) {
          Value* firstIdx = gepInstr->getOperand(1);

          if (ConstantInt *idx = dyn_cast<ConstantInt>(firstIdx)) {
            if (idx->getValue() == 0) {
              Value *replacer = gepInstr->getOperand(0);
              replaceOperand(func, gepInstr, replacer);
              gepInstr->eraseFromParent();
              return true;
            }
          }
        }
      }
    }
  }

  return false;
}

/**
 * un-inline ConstExpr in Functions' instructions
 */
void UnwrapGEP::handleFunctions(Module &M) {

  FunctionListType &funcList = M.getFunctionList();

  for (auto it = funcList.begin(); it != funcList.end(); ++it) {
    Function *func = &(*it);
    bool continueProcess = true;
    while (continueProcess) {
      continueProcess = processFunction(func);
    }
  }
}

bool UnwrapGEP::runOnModule(Module &M) {
  handleFunctions(M);

  return true;
}

bool UnwrapGEP::normalizeModule(Module &M) {
  UnwrapGEP pass;
  return pass.runOnModule(M);
}

static RegisterPass<UnwrapGEP> X("UnwrapGEP",
                                          "Normalize ConstantExpr",
                                          false /* Only looks at CFG */,
                                          false /* Analysis Pass */);

static RegisterStandardPasses Y(PassManagerBuilder::EP_EarlyAsPossible,
                                [](const PassManagerBuilder &Builder,
                                   legacy::PassManagerBase &PM) {
                                  PM.add(new UnwrapGEP());
                                });
