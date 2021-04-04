#include <iostream>

#include "llvm/IR/Type.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"

#include "llvm/InitializePasses.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

#include "llvm/Analysis/PostDominators.h"

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#include "llvm/Analysis/AliasAnalysis.h"

#include "Debug.h"
#include "Common.h"



using namespace std;
using namespace llvm;

namespace {

struct Discover : public ModulePass {
// struct Discover : public FunctionPass {

  static char ID;
  Discover() : ModulePass(ID) {}
  // Discover() : FunctionPass(ID) {}

  virtual void getAnalysisUsage(AnalysisUsage &AU) const;

  // bool runOnFunction(Function &F) override;
  bool runOnModule(Module &M) override;
};

} // namespace discover
