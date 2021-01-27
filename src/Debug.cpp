#include "Debug.h"

using namespace llvm;

bool llvm::debugging = false;

bool llvm::print_input_program = false;

bool llvm::print_output_program = false;

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
