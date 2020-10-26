#include <iostream>

#include "llvm/IR/Type.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"

#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

using namespace std;
using namespace llvm;

using GlobalListType = SymbolTableList<GlobalVariable>;
using FunctionListType = SymbolTableList<Function>;
using BasicBlockListType = SymbolTableList<BasicBlock>;

namespace discover {

struct UnwrapGEP : public ModulePass {
  static char ID;
  static bool normalizeModule(Module &M);

  void replaceOperand(Function *func, Value *oldOpr, Value *newOpr);
  bool processFunction(Function *func);

  void handleFunctions(Module &M);

  UnwrapGEP() : ModulePass(ID) {}

  virtual bool runOnModule(Module &M) override;
};

} // namespace discover