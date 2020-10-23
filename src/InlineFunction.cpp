#include "InlineFunction.h"

using namespace discover;
using namespace llvm;

char InlineFunction::ID = 0;

void InlineFunction::handleGlobals(Module &M) {
}

void InlineFunction::handleInstr(IRBuilder<> builder, Instruction* instr) {
}

void InlineFunction::handleFunctions(Module &M) {
}

bool InlineFunction::runOnModule(Module &M) {
  handleGlobals(M);

  handleFunctions(M);

  return true;
}

bool InlineFunction::normalizeModule(Module &M) {
  InlineFunction pass;
  return pass.runOnModule(M);
}

static RegisterPass<InlineFunction> X("InlineFunction",
                                          "Normalize ConstantExpr",
                                          false /* Only looks at CFG */,
                                          false /* Analysis Pass */);

static RegisterStandardPasses Y(PassManagerBuilder::EP_EarlyAsPossible,
                                [](const PassManagerBuilder &Builder,
                                   legacy::PassManagerBase &PM) {
                                  PM.add(new InlineFunction());
                                });
