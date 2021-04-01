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
#include "ElimIdenticalInstrs.h"
#include "CombineGEP.h"
#include "ElimAllocaStoreLoad.h"

using namespace std;
using namespace llvm;
using namespace discover;

typedef struct Arguments {
  string inputFile;
  string outputFile;
  bool normalizeAll;
  string inlineFunction;
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
    ("inline-function", "Inline specific function", cxxopts::value<std::string>())
    ("pip", "Print input program")
    ("pop", "Print output program")
    ("help", "Help");

  // normalize all by default
  args.normalizeAll = true;

  // parse arguments by positional and flags
  options.parse_positional({"input"});
  auto parsedOptions = options.parse(argc, argv);

  // get input file
  args.inputFile = parsedOptions["input"].as<std::string>();
  if (args.inputFile.empty()) {
    cerr << "Input file not found";
    exit(1);
  }
  cout << "Input File: " << args.inputFile << std::endl;

  // get output file
  if (parsedOptions.count("output")) {
    args.outputFile = parsedOptions["output"].as<std::string>();
    cout << "Output File: " << args.outputFile << std::endl;
  }

  // get inlining function
  if (parsedOptions.count("inline-function")) {
    args.normalizeAll = false;
    args.inlineFunction = parsedOptions["inline-function"].as<std::string>();
    cout << "Function to be inline: " << args.inlineFunction << std::endl;
  }


  // get some debugging flags
  debugging = parsedOptions["debug"].as<bool>();
  printInputProgram = parsedOptions["pip"].as<bool>();
  printOutputProgram = parsedOptions["pop"].as<bool>();

  return args;
}

void normalizeGlobal(Module& M) {
  InitGlobal::normalizeModule(M);
}


void normalizeFunction(Function& F) {
  UninlineInstruction::normalizeFunction(F);
  CombineGEP::normalizeFunction(F);
  ElimIdenticalInstrs::normalizeFunction(F);
  ElimAllocaStoreLoad::normalizeFunction(F);
}


void normalizeModule(Module& M) {
  ElimUnusedAuxFunction::normalizeModule(M);
  InlineSimpleFunction::normalizeModule(M);
  ElimUnusedGlobal::normalizeModule(M);
}


int main(int argc, char** argv) {
  cout << "LLVM Normalizer for Discover" << std::endl;

  Arguments args = parseArguments(argc, argv);
  string inputFile = args.inputFile;
  string outputFile = args.outputFile;

  llvm::outs() << "normalize all? " << args.normalizeAll << "\n";

  // process bitcode
  SMDiagnostic err;
  static LLVMContext context;

  std::unique_ptr<Module> M = parseIRFile(inputFile, err, context);

  if (printInputProgram) {
    debug() << "===============================\n"
            << "BEFORE NORMALIZATION:\n";
    M->print(debug(), nullptr);
  }

  // Normalize all
  if (args.normalizeAll) {
    // Normalize globals first
    normalizeGlobal(*M);

    // Run each FunctionPass
    FunctionList &FS = M->getFunctionList();
    for (Function &F: FS) {
      normalizeFunction(F);
    }

    // Run ModulePass
    normalizeModule(*M);
  }
  else {
    if (!args.inlineFunction.empty()) {
      string &funcName = args.inlineFunction;
      debug() << "<<<<<<<<<<<<<<<<<<<\n"
              << "Inline function: " << funcName << "\n";
      InlineSimpleFunction::inlineFunction(*M, funcName);
    }
  }

  if (printOutputProgram) {
    debug() << "===============================\n"
            << "AFTER NORMALIZATION:\n";
    M->print(debug(), nullptr);
  }

  // write output
  if (!outputFile.empty()) {
    std::error_code EC;
    raw_fd_ostream OS(outputFile, EC, llvm::sys::fs::F_None);
    WriteBitcodeToFile(*M, OS);
    OS.flush();
  }

  return 0;
}
