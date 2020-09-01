#include "UninlineConstExpr.h"

using namespace discover;
using namespace llvm;

char UninlineConstExpr::ID = 0;

bool UninlineConstExpr::runOnModule(Module &M) {
  // TODO: separate the un-inlining  into two modules:
  // 1. un-inline global vars
  // 2. un-inline instructions' operations

  // un-inline ConstExpr in global variables
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
  BasicBlock *blkInitGlobal = BasicBlock::Create(ctx, "", funcInitGlobal);
  IRBuilder<> builder(blkInitGlobal);

  for (auto it = globalList.begin(); it != globalList.end(); ++it) {
    GlobalVariable *global = &(*it);
    // outs() << "Global Var: " << *global << "\n";

    Constant* init = global->getInitializer();

    if (ConstantStruct * structInit = dyn_cast<ConstantStruct>(init)) {

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
  }

  // delete the initialization function when it is not needed
  if (blkInitGlobal->size() == 0)
    funcInitGlobal->eraseFromParent();


  // un-inline ConstExpr in module
  FunctionListType &funcList = M.getFunctionList();

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
        for (int i = 0; i < instr->getNumOperands(); i++) {
          Value *operand = instr->getOperand(i);
          if (ConstantExpr *expr = dyn_cast<ConstantExpr>(operand)) {
            Instruction *exprInstr = expr->getAsInstruction();
            builder.Insert(exprInstr);
            instr->setOperand(i, exprInstr);
          }
        }
      }
    }
  }

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
