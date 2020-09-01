#include "UninlineConstExpr.h"

using namespace discover;
using namespace llvm;

char UninlineConstExpr::ID = 0;

bool UninlineConstExpr::runOnModule(Module &M) {
  // un-inline ConstExpr in global variables
  GlobalListType &globalList = M.getGlobalList();
  // cout << "Normalizing Globals:\n";

  LLVMContext &ctx = M.getContext();

  // create a function to initialize global variables
  Type* retType = Type::getVoidTy(ctx);
  vector<Type*> argTypes;
  FunctionType *funcType = FunctionType::get(retType, argTypes, false);
  Function* funcInitGlobal = Function::Create(funcType,
                                              Function::ExternalLinkage,
                                              "InitGlobal",
                                              M);
  BasicBlock *blkInitGlobal = BasicBlock::Create(ctx, "entry", funcInitGlobal);
  IRBuilder<> builder(blkInitGlobal);

  for (auto it = globalList.begin(); it != globalList.end(); ++it) {
    GlobalVariable *global = &(*it);
    outs() << "Global Var: " << *global << "\n";

    Constant* init = global->getInitializer();

    outs() << "  Init: " << *init << "\n";

    if (ConstantStruct * structInit = dyn_cast<ConstantStruct>(init)) {
      outs() << "    Constant Struct" << "\n";
      outs() << "    Num of fields: " << structInit->getNumOperands() << "\n";

      for (int i = 0; i < structInit->getNumOperands(); i++) {
        Value *operand = structInit->getOperand(i);
        if (ConstantExpr *expr = dyn_cast<ConstantExpr>(operand)) {
          outs() << "    Operand " << i << ": " << *expr << "\n";

          string gName = global->getName();
          string gFName = gName + "_fld_" + to_string(i);

          // GlobalVariable* gFVar = new GlobalVariable(M, expr->getType(), false,
          //                                            GlobalValue::CommonLinkage,
          //                                            expr, gFName, global);

          if (PointerType* pTyp = dyn_cast<PointerType>(expr->getType())) {
            Constant* pnull = ConstantPointerNull::get(pTyp);
            // first set this field to a null pointer
            structInit->setOperand(i, pnull);
            // then create a function initialize this field
            Instruction *exprInstr = expr->getAsInstruction();
            builder.Insert(exprInstr);

            IntegerType* int32Typ = IntegerType::get(ctx, 32);
            ConstantInt* elemIdx = ConstantInt::get(int32Typ, 0);
            ConstantInt* ptrIdx = ConstantInt::get(int32Typ, i);
            Value* idxList[2] = {elemIdx, ptrIdx};
            Type* elemptrTyp = pTyp->getPointerTo();
            Instruction* gepInst = GetElementPtrInst::CreateInBounds(global,
                                                                 (ArrayRef<Value*>)idxList);
            builder.Insert(gepInst);
            Instruction* storeInst = new StoreInst(exprInstr, gepInst);
            builder.Insert(storeInst);
          }
        }
      }
    }
  }

  outs() << "New Global Variables\n";
  for (auto it = globalList.begin(); it != globalList.end(); ++it) {
    GlobalVariable *global = &(*it);
    outs() << "Global Var: " << *global << "\n";
  }

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
