#ifndef DEBUG_H
#define DEBUG_H

#include <iostream>
#include <stdarg.h>

#include "llvm/IR/Type.h"
#include "llvm/Support/raw_ostream.h"

namespace llvm {

extern bool debugging;
extern bool print_input_program;
extern bool print_output_program;

raw_ostream &debug();
raw_ostream &error();

}

#endif
