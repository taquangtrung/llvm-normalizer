#include "InitGlobal.h"

using namespace discover;
using namespace llvm;

char InitGlobal::ID = 0;


void InitGlobal::uninlineInitValue(LLVMContext &ctx,
                                   IRBuilder<> builder,
                                   GlobalVariable *global,
                                   std::vector<Value*> gepIdxs,
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

  // ConstExpr
  if (ConstantExpr *exprInit = dyn_cast<ConstantExpr>(initValue)) {
    // then initialize this field in the global initialization function
    Instruction *exprInstr = exprInit->getAsInstruction();
    builder.Insert(exprInstr);
    ArrayRef<Value*> idxs = (ArrayRef<Value*>)gepIdxs;
    Instruction* gepInst = GetElementPtrInst::CreateInBounds(global, idxs);
    builder.Insert(gepInst);
    Instruction* storeInst = new StoreInst(exprInstr, gepInst);
    builder.Insert(storeInst);
  }

  // Constant Integer
  else if (isa<ConstantInt>(initValue)) {
    ArrayRef<Value*> idxs = (ArrayRef<Value*>)gepIdxs;
    Instruction* gepInst = GetElementPtrInst::CreateInBounds(global, idxs);
    builder.Insert(gepInst);
    Instruction* storeInst = new StoreInst(initValue, gepInst);
    builder.Insert(storeInst);
  }

  // Constant Struct
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

/*
 * Init global variables capture in ‘llvm.global_ctors‘
 * See https://releases.llvm.org/6.0.1/docs/LangRef.html
 */
void InitGlobal::invokeGlobalInitFunctions(IRBuilder<> builder,
                                           GlobalVariable *global,
                                           Constant* initValue) {
  vector<pair<int, Function*>> prioFuncs;

  for (int i = 0; i < initValue->getNumOperands(); i++) {
    Value *operand = initValue->getOperand(i);
    if (ConstantStruct *st = dyn_cast<ConstantStruct>(operand)) {
      if (st->getNumOperands() == 3) {
        pair<int, Function*> prioFunc;
        if (ConstantInt* p = dyn_cast<ConstantInt>(st->getOperand(0)))
          prioFunc.first = p->getValue().getSExtValue();
        if (Function* f = dyn_cast<Function>(st->getOperand(1)))
          prioFunc.second = f;
        prioFuncs.push_back(prioFunc);
      }
    }
  }

  std::sort(prioFuncs.begin(), prioFuncs.end(),
            [](pair<int, Function*> fp1, pair<int, Function*> fp2) {
              return (fp1.first < fp2.first);});

  for (pair<int, Function*> fp : prioFuncs) {
    Function *func = fp.second;
    Instruction* callInst = CallInst::Create(func);
    builder.Insert(callInst);
  }

  return;
}

bool InitGlobal::runOnModule(Module &M) {

  GlobalVariableList &globalList = M.getGlobalList();
  LLVMContext &ctx = M.getContext();

  // create a function to initialize global variables
  Type* retType = Type::getVoidTy(ctx);
  vector<Type*> argTypes;
  FunctionType *funcType = FunctionType::get(retType, argTypes, false);
  Function* funcInit = Function::Create(funcType, Function::ExternalLinkage,
                                        0, "__init_globals", nullptr);
  funcInit->setDSOLocal(true);

  BasicBlock *blkInit = BasicBlock::Create(ctx, "", funcInit);
  IRBuilder<> builder(blkInit);

  for (auto it = globalList.begin(); it != globalList.end(); ++it) {
    GlobalVariable *global = &(*it);
    // outs() << "Global Var: " << *global << "\n";

    if (!(global->hasInitializer()))
      continue;

    StringRef globalName = global->getName();
    Constant* initValue = global->getInitializer();

    // call global vars constructors
    if (globalName.equals(LLVM_GLOBAL_CTORS)) {
      invokeGlobalInitFunctions(builder, global, initValue);
    }
    // uninline init values of globals
    else {
      std::vector<Value*> gepIdxs;
      gepIdxs.push_back(ConstantInt::get(IntegerType::get(ctx, 32), 0));
      uninlineInitValue(ctx, builder, global, gepIdxs, initValue);
    }
  }

  // delete the initialization function when it is not needed
  if (blkInit->size() != 0) {
    Instruction* returnInst = ReturnInst::Create(ctx);
    builder.Insert(returnInst);

    // insert the function to the beginning of the list
    FunctionList &funcList = M.getFunctionList();
    funcList.push_front(funcInit);
  }


  return true;
}

bool InitGlobal::normalizeModule(Module &M) {
  debug() << "\n=========================================\n"
          << "Flatten Globals ...\n";

  InitGlobal pass;
  return pass.runOnModule(M);
}

static RegisterPass<InitGlobal> X("InitGlobal",
                                          "Normalize ConstantExpr",
                                          false /* Only looks at CFG */,
                                          false /* Analysis Pass */);

static RegisterStandardPasses Y(PassManagerBuilder::EP_EarlyAsPossible,
                                [](const PassManagerBuilder &Builder,
                                   legacy::PassManagerBase &PM) {
                                  PM.add(new InitGlobal());
                                });
