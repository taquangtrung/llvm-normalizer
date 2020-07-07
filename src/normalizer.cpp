#include <iostream>

#include "llvm/IRReader/IRReader.h"

#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"

#include "llvm/Passes/PassBuilder.h"

// #include "NormalizeConstExpr.h"

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

using FunctionListType = SymbolTableList<Function>;
using BasicBlockListType = SymbolTableList<BasicBlock>;


bool normalizeModule(Module& module) {
  FunctionListType &funcList = module.getFunctionList();

  for (auto it = funcList.begin(); it != funcList.end(); ++it) {
    auto func = it;
    BasicBlockListType &blockList = func->getBasicBlockList();

    for (auto it2 = blockList.begin(); it2 != blockList.end(); ++it2) {
      BasicBlock *blk = &(*it2);
      IRBuilder<> builder(blk);

      for (auto it3 = blk->begin(); it3 != blk->end(); ++it3) {
        Instruction *instr = &(*it3);
        builder.SetInsertPoint(instr);

        // transform ConstantExpr in operands into new instructions
        // for (int i = 0; i < instr->getNumOperands(); i++) {
        //   Value *operand = instr->getOperand(i);
        //   if (ConstantExpr *expr = dyn_cast<ConstantExpr>(operand)) {
        //     Instruction *exprInstr = expr->getAsInstruction();
        //     builder.Insert(exprInstr);
        //     instr->setOperand(i, exprInstr);
        //   }
        // }
      }
    }
  }

  return false;
  // return NormalizeConstExpr::normalizeModule(module);
}


int main(int argc, char** argv) {

  cout << "Discover-Llvm Normalizer" << std::endl;

  unique_ptr<Module> module;
  SMDiagnostic err;
  LLVMContext context;

  string inputFileName = *(argv+1);
  cout << "Input File: " << *(argv+1) << std::endl;

  module = parseIRFile(inputFileName, err, context);

  // print module
  cout << "BEFORE NORMALIZATION: " << std::endl;
  // module->print(outs(), nullptr);

  // NormalizeConstExpr *a;
  // a->runOnModule(*module);

  // normalizeModule(*module);

  // cout << "AFTER NORMALIZATION: " << std::endl;
  // module->print(outs(), nullptr);

  return 1;
}
