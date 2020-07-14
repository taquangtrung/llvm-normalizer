#include <iostream>

#include "llvm/IRReader/IRReader.h"
#include "llvm/Bitcode/BitcodeWriter.h"

#include "llvm/Support/CommandLine.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"

// #include "cxxopts/cxxopts.hpp"
#include "cxxopts/cxxopts.hpp"

#include "UninlineConstExpr.h"

using namespace std;
using namespace llvm;
using namespace discover;

typedef struct Arguments {
  string inputFile;
  string outputFile;
  bool debugging;
} Arguments;

Arguments parseArguments(int argc, char** argv) {
  Arguments args;

  // Parse arguments
  cxxopts::Options options("normalizer", "Options of normalizer");
  options.add_options()
    ("input", "Input file", cxxopts::value<std::string>())
    ("output", "Output file", cxxopts::value<std::string>())
    ("debug", "Enable debugging")
    ;

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

  // get debugging flag
  args.debugging = parsedOptions["debug"].as<bool>();

  return args;
}

bool normalizeModule(Module& module) {
  return UninlineConstExpr::normalizeModule(module);
}


int main(int argc, char** argv) {
  cout << "Discover-Llvm Normalizer" << std::endl;

  Arguments args = parseArguments(argc, argv);
  string inputFile = args.inputFile;
  string outputFile = args.outputFile;
  bool debugging = args.debugging;

  // process bitcode
  unique_ptr<Module> module;
  SMDiagnostic err;
  static LLVMContext context;

  module = parseIRFile(inputFile, err, context);

  if (debugging) {
    cout << "===============================\n"
         << "BEFORE NORMALIZATION: " << std::endl;
    module->print(outs(), nullptr);
  }

  // normalize module
  normalizeModule(*module);

  if (debugging) {
    cout << "===============================\n"
         << "AFTER NORMALIZATION: " << std::endl;
    module->print(outs(), nullptr);
  }

  // write output
  if (!outputFile.empty()) {
    std::error_code EC;
    raw_fd_ostream OS(outputFile, EC, llvm::sys::fs::F_None);
    WriteBitcodeToFile(*module, OS);
    OS.flush();
  }

  return 0;
}
