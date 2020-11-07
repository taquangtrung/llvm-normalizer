#include "UninlineInstruction.h"

using namespace discover;
using namespace llvm;

char UninlineInstruction::ID = 0;


/**
 * un-inline ConstExpr in instructions, recursively
 */
void UninlineInstruction::uninlineConstExpr(IRBuilder<> builder, Instruction* instr) {
  builder.SetInsertPoint(instr);

  // transform ConstantExpr in operands into new instructions
  for (int i = 0; i < instr->getNumOperands(); i++) {
    Value *operand = instr->getOperand(i);
    if (ConstantExpr *expr = dyn_cast<ConstantExpr>(operand)) {
      // outs() << "ConstantExpr: " << *expr << "\n";

      Instruction *exprInstr = expr->getAsInstruction();
      // outs() << "ConstantExprInstr: " << *exprInstr << "\n";

      builder.Insert(exprInstr);
      instr->setOperand(i, exprInstr);

      uninlineConstExpr(builder, exprInstr);
    }
  }
}

bool UninlineInstruction::runOnModule(Module &M) {
  FunctionList &funcList = M.getFunctionList();

  for (auto it = funcList.begin(); it != funcList.end(); ++it) {
    auto func = it;

    BasicBlockList &blockList = func->getBasicBlockList();

    for (auto it2 = blockList.begin(); it2 != blockList.end(); ++it2) {
      BasicBlock *blk = &(*it2);
      IRBuilder<> builder(blk);

      for (auto it3 = blk->begin(); it3 != blk->end(); ++it3) {
        Instruction *instr = &(*it3);

        uninlineConstExpr(builder, instr);
      }
    }
  }

  return true;
}

bool UninlineInstruction::normalizeModule(Module &M) {
  debug() << "\n=========================================\n"
          << "Uninlining Globals and Instructions ...\n";

  UninlineInstruction pass;
  return pass.runOnModule(M);
}

static RegisterPass<UninlineInstruction> X("UninlineInstruction",
                                          "Normalize ConstantExpr",
                                          false /* Only looks at CFG */,
                                          false /* Analysis Pass */);

static RegisterStandardPasses Y(PassManagerBuilder::EP_EarlyAsPossible,
                                [](const PassManagerBuilder &Builder,
                                   legacy::PassManagerBase &PM) {
                                  PM.add(new UninlineInstruction());
                                });
