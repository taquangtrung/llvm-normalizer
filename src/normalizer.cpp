#include <iostream>

#include "llvm-c/Core.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"

using namespace std;
using namespace llvm;

void preprocess_constant_expr(llvm::Module* module) {
  for (auto it = module->getFunctionList().begin();
       it != module->getFunctionList().end();
       ++it) {
    auto func = it;

    for (auto it2 = func->getBasicBlockList().begin();
         it2 != func->getBasicBlockList().end();
         ++it2) {
      BasicBlock* blk = &(*it2);

      IRBuilder<> builder(blk);

      for (auto it3 = blk->begin(); it3 != blk->end(); ++it3) {
        Instruction* instr = &(*it3);

        builder.SetInsertPoint(instr);

        for (int i = 0; i < instr->getNumOperands(); i++) {
          Value* operand = instr->getOperand(i);
          if (ConstantExpr* expr = dyn_cast<ConstantExpr>(operand)) {
            Instruction* exprInstr = expr->getAsInstruction();
            builder.Insert(exprInstr);
            instr->setOperand(i, exprInstr);
          }
        }
      }
    }
  }
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
  cout << "Before normalization: " << std::endl;
  module->print(outs(), nullptr);

  preprocess_constant_expr(&(*module));

  cout << "After normalization: " << std::endl;
  module->print(outs(), nullptr);

  // for (auto it = module->getFunctionList().begin();
  //      it != module->getFunctionList().end();
  //      ++it) {
  //   auto func = it;
  //   outs() << "Function: " << func->getName().data() << "\n";
  //   for (auto it2 = func->getBasicBlockList().begin();
  //        it2 != func->getBasicBlockList().end();
  //        ++it2) {
  //     auto blk = it2;
  //     cout << "\n";
  //     for (auto it3 = blk->begin(); it3 != blk->end(); ++it3) {
  //       auto instr = it3;
  //       outs() << "  inst data: " << *instr << "\n";
  //       outs() << "  inst name: " << instr->getName().data() << "\n";
  //       outs() << "  inst op: "
  //                    << instr->getOperand(0)->getName()
  //                    << "\n";
  //       if (MDNode *N = instr->getMetadata("dbg")) {  // Here I is an LLVM instruction
  //         outs() << "          has metadata\n";
  //       }
  //     }
  //   }
  // }

}
