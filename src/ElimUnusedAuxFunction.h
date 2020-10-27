#include <iostream>

#include "llvm/IR/Type.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/CallSite.h"
#include "llvm/IR/Verifier.h"

#include "llvm/Analysis/InlineCost.h"

#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/Utils/Cloning.h"

using namespace std;
using namespace llvm;

using GlobalListType = SymbolTableList<GlobalVariable>;
using FunctionListType = SymbolTableList<Function>;
using BasicBlockListType = SymbolTableList<BasicBlock>;

namespace discover {

struct ElimUnusedAuxFunction : public ModulePass {
  static char ID;
  static bool normalizeModule(Module &M);

  ElimUnusedAuxFunction() : ModulePass(ID) {}

  virtual bool runOnModule(Module &M) override;
};

} // namespace discover