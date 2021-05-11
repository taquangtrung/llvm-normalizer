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

using ASLInstrs = std::tuple<AllocaInst*, StoreInst*, std::vector<LoadInst*>>;

struct ElimAllocaStoreLoad : public FunctionPass {
  static char ID;

  ElimAllocaStoreLoad() : FunctionPass(ID) {}
  virtual bool runOnFunction(Function &F) override;

private:

  void removeAllocaStoreLoad(Function&, std::vector<ASLInstrs>);
  std::vector<ASLInstrs> findRemovableAllocaStoreLoad(Function&);

};

} // namespace discover
