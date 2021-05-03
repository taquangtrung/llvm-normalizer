#include <iostream>

#include "llvm/IR/Type.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/Verifier.h"

#include "llvm/Analysis/InlineCost.h"

#include "llvm/Analysis/PostDominators.h"

#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/Utils/Cloning.h"

#include "Debug.h"
#include "Common.h"

using namespace std;
using namespace llvm;

namespace discover {

struct ElimUnusedGlobal : public ModulePass {
  static char ID;
  ElimUnusedGlobal() : ModulePass(ID) {}

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    debug() << "ElimUnusedGlobal: Add Pass: DominatorTreeWrapperPass\n";
    AU.addRequired<DominatorTreeWrapperPass>();
  }

  void releaseMemory() override {
    debug() << "ElimUnusedGlobal: Remove Pass: DominatorTreeWrapperPass\n";
  }

  virtual bool runOnModule(Module &M) override;
};

} // namespace discover
