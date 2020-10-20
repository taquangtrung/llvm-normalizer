#include "UninlineConstExpr.h"

using namespace discover;
using namespace llvm;

char UninlineConstExpr::ID = 0;

/**
 * un-inline ConstExpr in global variables' initializers
 */
void UninlineConstExpr::handleGlobals(Module &M) {
  outs() << "Handling globals initialization" << "\n";

  GlobalListType &globalList = M.getGlobalList();
  LLVMContext &ctx = M.getContext();

  // create a function to initialize global variables
  Type* retType = Type::getVoidTy(ctx);
  vector<Type*> argTypes;
  FunctionType *funcType = FunctionType::get(retType, argTypes, false);
  Function* funcInitGlobal = Function::Create(funcType,
                                              Function::ExternalLinkage,
                                              "__init_globals",
                                              M);
  funcInitGlobal->setDSOLocal(true);

  BasicBlock *blkInitGlobal = BasicBlock::Create(ctx, "", funcInitGlobal);
  IRBuilder<> builder(blkInitGlobal);

  for (auto it = globalList.begin(); it != globalList.end(); ++it) {
    GlobalVariable *global = &(*it);
    // outs() << "Global Var: " << *global << "\n";

    Constant* init = global->getInitializer();

    if (init == NULL) {
      continue;
    }

    //
    else if (ConstantStruct * structInit = dyn_cast<ConstantStruct>(init)) {
      outs() << "ConstantStruct of " << *global << "\n";

      for (int i = 0; i < structInit->getNumOperands(); i++) {
        Value *operand = structInit->getOperand(i);
        if (ConstantExpr *expr = dyn_cast<ConstantExpr>(operand)) {
          if (PointerType* pTyp = dyn_cast<PointerType>(expr->getType())) {
            // first set this field to a null pointer
            structInit->setOperand(i, ConstantPointerNull::get(pTyp));

            // then initialize this field in the global initialization function
            Instruction *exprInstr = expr->getAsInstruction();
            builder.Insert(exprInstr);

            IntegerType* int32Typ = IntegerType::get(ctx, 32);
            ConstantInt* elemIdx = ConstantInt::get(int32Typ, 0);
            ConstantInt* ptrIdx = ConstantInt::get(int32Typ, i);
            Value* idxList[2] = {elemIdx, ptrIdx};
            ArrayRef<Value*> gepIdx = (ArrayRef<Value*>)idxList;
            Instruction* gepInst = GetElementPtrInst:: CreateInBounds(global, gepIdx);
            builder.Insert(gepInst);

            Instruction* storeInst = new StoreInst(exprInstr, gepInst);
            builder.Insert(storeInst);
          }
        }
      }
    }

    //
    else if (ConstantArray * arrayInit = dyn_cast<ConstantArray>(init)) {
      outs() << "ConstantArray" << "\n";
      outs() << "Num operands: " << arrayInit->getNumOperands() << "\n";
      for (int i = 0; i < arrayInit->getNumOperands(); i++) {
        Value *operand = arrayInit->getOperand(0);
        outs() << operand << "\n";
      }
    }

    //
    else if (ConstantVector * vectorInit = dyn_cast<ConstantVector>(init)) {
      outs() << "ConstantVector" << "\n";
      // outs() << "Num operands: " << arrayInit->getNumOperands() << "\n";
      // for (int i = 0; i < arrayInit->getNumOperands(); i++) {
      //   Value *operand = arrayInit->getOperand(0);
      //   outs() << operand << "\n";
      // }
    }


  }

  // delete the initialization function when it is not needed
  if (blkInitGlobal->size() == 0)
    funcInitGlobal->eraseFromParent();
  else {
    Instruction* returnInst = ReturnInst::Create(ctx);
    builder.Insert(returnInst);
  }
}

/**
 * un-inline ConstExpr in instructions, recursively
 */
void UninlineConstExpr::handleInstr(IRBuilder<> builder, Instruction* instr) {
  builder.SetInsertPoint(instr);

  // transform ConstantExpr in operands into new instructions
  for (int i = 0; i < instr->getNumOperands(); i++) {
    Value *operand = instr->getOperand(i);
    if (ConstantExpr *expr = dyn_cast<ConstantExpr>(operand)) {
      // outs() << "ConstantExpr: " << *expr << "\n";

      Instruction *exprInstr = expr->getAsInstruction();
      // outs() << "ConstantExprInstr: " << *exprInstr << "\n";

      builder.Insert(exprInstr);
      instr->setOperand(i, exprInstr);

      handleInstr(builder, exprInstr);
    }
  }
}

/**
 * un-inline ConstExpr in Functions' instructions
 */
void UninlineConstExpr::handleFunctions(Module &M) {

  FunctionListType &funcList = M.getFunctionList();

  for (auto it = funcList.begin(); it != funcList.end(); ++it) {
    auto func = it;
    BasicBlockListType &blockList = func->getBasicBlockList();

    for (auto it2 = blockList.begin(); it2 != blockList.end(); ++it2) {
      BasicBlock *blk = &(*it2);
      IRBuilder<> builder(blk);

      for (auto it3 = blk->begin(); it3 != blk->end(); ++it3) {
        Instruction *instr = &(*it3);

        handleInstr(builder, instr);
      }
    }
  }
}

bool UninlineConstExpr::runOnModule(Module &M) {
  handleGlobals(M);

  handleFunctions(M);

  return true;
}

bool UninlineConstExpr::normalizeModule(Module &M) {
  UninlineConstExpr pass;
  return pass.runOnModule(M);
}

static RegisterPass<UninlineConstExpr> X("UninlineConstExpr",
                                          "Normalize ConstantExpr",
                                          false /* Only looks at CFG */,
                                          false /* Analysis Pass */);

static RegisterStandardPasses Y(PassManagerBuilder::EP_EarlyAsPossible,
                                [](const PassManagerBuilder &Builder,
                                   legacy::PassManagerBase &PM) {
                                  PM.add(new UninlineConstExpr());
                                });
