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

#include "Debug.h"
#include "Common.h"

using namespace std;
using namespace llvm;

namespace discover {

struct UninlineGlobal : public ModulePass {
  static char ID;
  static bool normalizeModule(Module &M);

  void uninlineInitValue(LLVMContext &ctx, IRBuilder<> builder,
                         GlobalVariable *global, std::vector<Value*> gepIdxs,
                         Constant* initValue);

  UninlineGlobal() : ModulePass(ID) {}

  virtual bool runOnModule(Module &M) override;
};

} // namespace discover
