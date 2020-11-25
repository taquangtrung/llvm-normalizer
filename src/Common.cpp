#include "Common.h"

using namespace llvm;

bool llvm::isTestingFunc(Function *F) {
  StringRef fName = F->getName();

  if (fName.contains("__assert") || fName.contains("__retute"))
    return true;

  return false;
}
