#include <iostream>

#include "llvm/IR/Type.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/GlobalValue.h"

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

struct InlineComplFunction : public ModulePass {
  static char ID;
  static bool normalizeModule(Module &M);

  Function* findInlineableFunc(Module &M);
  void inlineFunction(Module &M, Function* func);

  InlineComplFunction() : ModulePass(ID) {}

  virtual bool runOnModule(Module &M) override;
};

} // namespace discover
