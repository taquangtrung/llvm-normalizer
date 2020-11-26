#include "Common.h"
#include "Debug.h"

using namespace llvm;

bool llvm::isDiscoverTestingFunc(Function &F) {
  StringRef fName = F.getName();

  if (fName.contains("__assert") || fName.contains("__retute"))
    return true;

  return false;
}
