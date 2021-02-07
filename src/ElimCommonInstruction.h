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

struct ElimCommonInstruction : public ModulePass {
  static char ID;
  static bool normalizeModule(Module &M);

  GetElementPtrInst* findGEPOfSameElement(GetElementPtrInst *instr);
  PHINode* findPHINodeOfSameIncoming(PHINode *instr);
  CastInst* findCastInstOfCommonSource(CastInst *instr);

  bool processFunction(Function *func);
  void handleFunctions(Module &M);

  ElimCommonInstruction() : ModulePass(ID) {}

  virtual bool runOnModule(Module &M) override;
};

} // namespace discover
