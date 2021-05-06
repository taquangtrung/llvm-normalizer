#include "Debug.h"

using namespace llvm;

// initialize external variables
bool llvm::debugging = false;
bool llvm::printInputProgram = false;
bool llvm::printOutputProgram = false;

raw_ostream &llvm::debug() {
  if (debugging)
    return outs();
  else
    return nulls();
}

raw_ostream &llvm::error() {
  if (debugging)
    return errs();
  else
    return nulls();
}
