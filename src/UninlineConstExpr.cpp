#include "UninlineConstExpr.h"

using namespace discover;
using namespace llvm;

char UninlineConstExpr::ID = 0;


void UninlineConstExpr::processGlobalInitValue(LLVMContext &ctx, IRBuilder<> builder,
                                               GlobalVariable *global, std::vector<Value*> gepIdxs,
                                               Constant* initValue) {
  debug() << "++ Processing global: " << *global << "\n"
          << "   Current indices: ";
  for (Value* idx: gepIdxs) {
    debug() << *idx << ",  ";
  }
  debug() << "\n";
  debug() << "   Init value: " << *initValue << "\n";

  if (initValue->isNullValue() || initValue->isZeroValue())
    return;

  if (ConstantExpr *exprInit = dyn_cast<ConstantExpr>(initValue)) {
    // then initialize this field in the global initialization function
    Instruction *exprInstr = exprInit->getAsInstruction();
    builder.Insert(exprInstr);

    Instruction* gepInst = GetElementPtrInst::CreateInBounds(global, (ArrayRef<Value*>)gepIdxs);
    builder.Insert(gepInst);

    Instruction* storeInst = new StoreInst(exprInstr, gepInst);
    builder.Insert(storeInst);
  }
  else if (isa<ConstantInt>(initValue)) {
    Instruction* gepInst = GetElementPtrInst::CreateInBounds(global, (ArrayRef<Value*>)gepIdxs);
    builder.Insert(gepInst);

    Instruction* storeInst = new StoreInst(initValue, gepInst);
    builder.Insert(storeInst);
  }
  else if (ConstantStruct * structInit = dyn_cast<ConstantStruct>(initValue)) {
    for (int i = 0; i < structInit->getNumOperands(); i++) {
      Constant *fieldInit = structInit->getOperand(i);

      if (fieldInit->isNullValue() || fieldInit->isZeroValue())
        continue;

      Value* fieldIdx = ConstantInt::get(IntegerType::get(ctx, 32), i);
      std::vector<Value*> currentIdxs(gepIdxs);
      currentIdxs.push_back(fieldIdx);

      debug() << "  field init: " << *fieldInit << "\n";

      Type* fieldTyp = fieldInit->getType();

      if (PointerType* ptrTyp = dyn_cast<PointerType>(fieldTyp))
        structInit->setOperand(i, ConstantPointerNull::get(ptrTyp));
      else if (IntegerType* intTyp = dyn_cast<IntegerType>(fieldTyp))
        structInit->setOperand(i, ConstantInt::get(intTyp, 0));

      processGlobalInitValue(ctx, builder, global, currentIdxs, fieldInit);

    }
  }

  // unline a struct
  else if (ConstantArray * arrayInit = dyn_cast<ConstantArray>(initValue)) {
    for (int i = 0; i < arrayInit->getNumOperands(); i++) {
      Constant *elemInit = arrayInit->getOperand(i);

      if (elemInit->isNullValue() || elemInit->isZeroValue())
        continue;

      Value* elemIdx = ConstantInt::get(IntegerType::get(ctx, 32), i);
      std::vector<Value*> currentIdxs(gepIdxs);
      currentIdxs.push_back(elemIdx);

      if (PointerType* elemTyp = dyn_cast<PointerType>(elemInit->getType()))
        arrayInit->setOperand(i, ConstantPointerNull::get(elemTyp));
      else if (IntegerType* intTyp = dyn_cast<IntegerType>(elemTyp))
        structInit->setOperand(i, ConstantInt::get(intTyp, 0));

      processGlobalInitValue(ctx, builder, global, currentIdxs, elemInit);

    }
  }

}

/**
 * un-inline ConstExpr in global variables' initializers
 */
void UninlineConstExpr::handleGlobals(Module &M) {
  // outs() << "Handling globals initialization" << "\n";

  GlobalVariableList &globalList = M.getGlobalList();
  LLVMContext &ctx = M.getContext();

  // create a function to initialize global variables
  Type* retType = Type::getVoidTy(ctx);
  vector<Type*> argTypes;
  FunctionType *funcType = FunctionType::get(retType, argTypes, false);
  Function* funcInitGlobal = Function::Create(funcType, Function::ExternalLinkage,
                                          0, "__init_globals", nullptr);
  funcInitGlobal->setDSOLocal(true);

  BasicBlock *blkInitGlobal = BasicBlock::Create(ctx, "", funcInitGlobal);
  IRBuilder<> builder(blkInitGlobal);

  for (auto it = globalList.begin(); it != globalList.end(); ++it) {
    GlobalVariable *global = &(*it);
    // outs() << "Global Var: " << *global << "\n";

    if (!(global->hasInitializer()))
      continue;

    Constant* init = global->getInitializer();
    std::vector<Value*> gepIdxs;
    gepIdxs.push_back(ConstantInt::get(IntegerType::get(ctx, 32), 0));
    processGlobalInitValue(ctx, builder, global, gepIdxs, init);

  }

  // delete the initialization function when it is not needed
  if (blkInitGlobal->size() != 0) {
    Instruction* returnInst = ReturnInst::Create(ctx);
    builder.Insert(returnInst);

    // insert the function to the beginning of the list
    FunctionList &funcList = M.getFunctionList();
    funcList.push_front(funcInitGlobal);
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

  FunctionList &funcList = M.getFunctionList();

  for (auto it = funcList.begin(); it != funcList.end(); ++it) {
    auto func = it;
    BasicBlockList &blockList = func->getBasicBlockList();

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
  debug() << "\n=========================================\n"
          << "Uninlining Globals and Instructions ...\n";

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
