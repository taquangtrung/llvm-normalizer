#include "InlineComplFunction.h"

using namespace discover;
using namespace llvm;

char InlineComplFunction::ID = 0;

Function* InlineComplFunction::findInlineableFunc(Module &M) {
  FunctionListType &funcList = M.getFunctionList();

  for (auto it = funcList.begin(); it != funcList.end(); ++it) {
    Function *func = &(*it);

    // outs() << "Function: " << func->getName() << "\n";
    GlobalValue::LinkageTypes linkage = func->getLinkage();

    if (GlobalValue::isLinkOnceLinkage(linkage)) {
      // outs() << "  linked once!\n";
      return func;
    }
  }

  return NULL;
}

void InlineComplFunction::inlineFunction(Module &M, Function* func) {
  outs() << "Start to inline function: " << func->getName() << "\n";

  StringRef funcName = func->getName();

  llvm::InlineFunctionInfo ifi;

  FunctionListType &funcList = M.getFunctionList();

  for (auto it = funcList.begin(); it != funcList.end(); ++it) {
    Function *func = &(*it);
    BasicBlockListType &blockList = func->getBasicBlockList();

    for (auto it2 = blockList.begin(); it2 != blockList.end(); ++it2) {
      BasicBlock *blk = &(*it2);
      IRBuilder<> builder(blk);

      for (auto it3 = blk->begin(); it3 != blk->end(); ++it3) {
        Instruction *instr = &(*it3);
        if (CallInst *call = dyn_cast<CallInst>(instr)) {
          Function *calledFunc = call->getCalledFunction();
          outs() << "Called function: " << (calledFunc->getName()) << "\n";
          if (calledFunc->getName().equals(funcName)) {
            outs() << "  inlined!\n";
            InlineFunction(call, ifi);
          }
        }
      }
    }
  }

  func->eraseFromParent();
}

bool InlineComplFunction::runOnModule(Module &M) {
  // while (true) {
    Function *func = findInlineableFunc(M);

    // if (!func)
    //   break;

    inlineFunction(M, func);
  // }

  return true;
}

bool InlineComplFunction::normalizeModule(Module &M) {
  InlineComplFunction pass;
  return pass.runOnModule(M);
}

static RegisterPass<InlineComplFunction> X("InlineComplFunction",
                                      "Normalize ConstantExpr",
                                      false /* Only looks at CFG */,
                                      false /* Analysis Pass */);

static RegisterStandardPasses Y(PassManagerBuilder::EP_EarlyAsPossible,
                                [](const PassManagerBuilder &Builder,
                                   legacy::PassManagerBase &PM) {
                                  PM.add(new InlineComplFunction());
                                });
