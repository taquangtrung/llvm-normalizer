#include "UninlineGlobal.h"

using namespace discover;
using namespace llvm;

char UninlineGlobal::ID = 0;


void UninlineGlobal::uninlineInitValue(LLVMContext &ctx, IRBuilder<> builder,
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

      uninlineInitValue(ctx, builder, global, currentIdxs, fieldInit);

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

      uninlineInitValue(ctx, builder, global, currentIdxs, elemInit);

    }
  }
}


bool UninlineGlobal::runOnModule(Module &M) {
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
    uninlineInitValue(ctx, builder, global, gepIdxs, init);

  }

  // delete the initialization function when it is not needed
  if (blkInitGlobal->size() != 0) {
    Instruction* returnInst = ReturnInst::Create(ctx);
    builder.Insert(returnInst);

    // insert the function to the beginning of the list
    FunctionList &funcList = M.getFunctionList();
    funcList.push_front(funcInitGlobal);
  }
  return true;
}

bool UninlineGlobal::normalizeModule(Module &M) {
  debug() << "\n=========================================\n"
          << "Flatten Globals ...\n";

  UninlineGlobal pass;
  return pass.runOnModule(M);
}

static RegisterPass<UninlineGlobal> X("UninlineGlobal",
                                          "Normalize ConstantExpr",
                                          false /* Only looks at CFG */,
                                          false /* Analysis Pass */);

static RegisterStandardPasses Y(PassManagerBuilder::EP_EarlyAsPossible,
                                [](const PassManagerBuilder &Builder,
                                   legacy::PassManagerBase &PM) {
                                  PM.add(new UninlineGlobal());
                                });
