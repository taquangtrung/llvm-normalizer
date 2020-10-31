#include <iostream>

#include "llvm/IR/Type.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/CallSite.h"
#include "llvm/IR/LegacyPassManager.h"


using namespace std;
using namespace llvm;

using GlobalList = SymbolTableList<GlobalVariable>;
using FunctionList = SymbolTableList<Function>;
using BasicBlockList = SymbolTableList<BasicBlock>;

using FunctionSet = SmallSetVector<Function*, 16>;
