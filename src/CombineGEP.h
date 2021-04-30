#include <iostream>

#include "llvm/IR/Type.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"

#include "llvm/Pass.h"
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

struct CombineGEP : public FunctionPass {
  static char ID;

  CombineGEP() : FunctionPass(ID) {}

  virtual bool runOnFunction(Function &F) override;
};

} // namespace discover
