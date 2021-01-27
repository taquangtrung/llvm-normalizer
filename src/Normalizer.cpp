#include <iostream>

#include "llvm/IRReader/IRReader.h"
#include "llvm/Bitcode/BitcodeWriter.h"

#include "llvm/Support/CommandLine.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"

// #include "llvm/Transforms/AggressiveInstCombine/AggressiveInstCombine.h"

// #include "cxxopts/cxxopts.hpp"
#include "cxxopts/cxxopts.hpp"

#include "Debug.h"
#include "InitGlobal.h"
#include "UninlineInstruction.h"
#include "InlineSimpleFunction.h"
#include "ElimUnusedAuxFunction.h"
#include "ElimUnusedGlobal.h"
#include "ElimCommonPHI.h"

using namespace std;
using namespace llvm;
using namespace discover;

typedef struct Arguments {
  string inputFile;
  string outputFile;
  bool debugging;
  bool print_input_program;
  bool print_output_program;
} Arguments;

Arguments parseArguments(int argc, char** argv) {
  Arguments args;

  // Parse arguments
  cxxopts::Options options("normalizer", "Options of normalizer");
  options.add_options()
    ("input", "Input file", cxxopts::value<std::string>())
    ("o", "Output file", cxxopts::value<std::string>())
    ("output", "Output file", cxxopts::value<std::string>())
    ("debug", "Enable debugging")
    ("pip", "Print input program")
    ("pop", "Print output program")
    ("help", "Help");

  // parse arguments by positional and flags
  options.parse_positional({"input"});
  auto parsedOptions = options.parse(argc, argv);

  // get input file
  string inputFile = parsedOptions["input"].as<std::string>();
  if (inputFile.empty()) {
    cerr << "Input file not found";
    exit(1);
  }
  args.inputFile = inputFile;
  cout << "Input File: " << inputFile << std::endl;

  // get output file
  std::string outputFile;
  if (parsedOptions.count("output"))
    outputFile = parsedOptions["output"].as<std::string>();
  if (!outputFile.empty()) {
    args.outputFile = outputFile;
    cout << "Output File: " << outputFile << std::endl;
  }

  // get some debugging flags
  debugging = parsedOptions["debug"].as<bool>();
  print_input_program = parsedOptions["pip"].as<bool>();
  print_output_program = parsedOptions["pop"].as<bool>();

  return args;
}

// void normalizeFunction(Function& F) {
//   FunctionPass *instCombinePass = createAggressiveInstCombinerPass();
//   instCombinePass->runOnFunction(F);
// }

void normalizeModule(Module& M) {
  ElimUnusedAuxFunction::normalizeModule(M);
  InlineSimpleFunction::normalizeModule(M);
  InitGlobal::normalizeModule(M);
  UninlineInstruction::normalizeModule(M);
  ElimCommonPHI::normalizeModule(M);
  ElimUnusedGlobal::normalizeModule(M);
}


int main(int argc, char** argv) {
  cout << "Discover-Llvm Normalizer" << std::endl;

  Arguments args = parseArguments(argc, argv);
  string inputFile = args.inputFile;
  string outputFile = args.outputFile;

  // process bitcode
  // unique_ptr<Module> M;
  SMDiagnostic err;
  static LLVMContext context;

  std::unique_ptr<Module> M = parseIRFile(inputFile, err, context);

  if (print_input_program) {
    debug() << "===============================\n"
            << "BEFORE NORMALIZATION:\n";
    M->print(debug(), nullptr);
  }

  // FunctionList &Funcs = M->getFunctionList();
  // for (Function &F : Funcs) {
  //   normalizeFunction(F);
  // }

  normalizeModule(*M);

  if (print_output_program) {
    debug() << "===============================\n"
            << "AFTER NORMALIZATION:\n";
    M->print(debug(), nullptr);
  }

  // std::string str;
  // llvm::raw_string_ostream rso(str);
  // if (verifyModule(M, &rso)) {
  //   debug() << "After Normalizing\n";
  //   debug() << "Module is broken: " << rso.str() << "\n";
  //   debug() << M;
  // }

  // write output
  if (!outputFile.empty()) {
    std::error_code EC;
    raw_fd_ostream OS(outputFile, EC, llvm::sys::fs::F_None);
    WriteBitcodeToFile(*M, OS);
    OS.flush();
  }

  return 0;
}
