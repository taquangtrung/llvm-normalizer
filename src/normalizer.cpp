#include <iostream>

#include "llvm/IRReader/IRReader.h"
#include "llvm/Bitcode/BitcodeWriter.h"

#include "llvm/Support/CommandLine.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"

#include "cxxopts/cxxopts.hpp"

#include "NormalizeConstExpr.h"

using namespace std;
using namespace llvm;
using namespace discover;

bool normalizeModule(Module& module) {
  return NormalizeConstExpr::normalizeModule(module);
}


int main(int argc, char** argv) {

  cout << "Discover-Llvm Normalizer" << std::endl;

  cxxopts::Options options("LLVM-Normalizer", "One line description of Discover");

  options.add_options()
    ("out", "Output File", cxxopts::value<std::string>()) ;

  auto parsedOptions = options.parse(argc, argv);

  std::string outputFile;
  if (parsedOptions.count("out"))
    outputFile = parsedOptions["out"].as<std::string>();

  cout << "Output File: " << outputFile << std::endl;

  unique_ptr<Module> module;
  SMDiagnostic err;
  static LLVMContext context;

  string inputFileName = *(argv+1);
  cout << "Input File: " << inputFileName << std::endl;

  module = parseIRFile(inputFileName, err, context);

  // print module
  cout << "===============================\n"
       << "BEFORE NORMALIZATION: " << std::endl;
  module->print(outs(), nullptr);

  normalizeModule(*module);

  std::error_code EC;
  raw_fd_ostream OS("module", EC, llvm::sys::fs::F_None);

  WriteBitcodeToFile(*module, OS);
  OS.flush();

  cout << "===============================\n"
       << "AFTER NORMALIZATION: " << std::endl;
  module->print(outs(), nullptr);

  return 1;
}
