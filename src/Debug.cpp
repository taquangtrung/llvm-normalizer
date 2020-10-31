#include "Debug.h"

using namespace llvm;

bool llvm::debugging = false;

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
