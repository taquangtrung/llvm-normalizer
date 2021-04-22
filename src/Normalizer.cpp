#include <iostream>

#include "llvm/IRReader/IRReader.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/IRPrintingPasses.h"

#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/Bitcode/BitcodeWriterPass.h"

#include "llvm/Support/CommandLine.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"

#include "llvm/Transforms/Utils/Debugify.h"

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

static cl::opt<bool> DebugifyEach(
    "debugify-each",
    cl::desc("Start each pass with debugify and end it with check-debugify"));

class NormalizerCustomPassManager : public legacy::PassManager {
  DebugifyStatsMap DIStatsMap;

public:
  using super = legacy::PassManager;

  void add(Pass *P) override {
    // Wrap each pass with (-check)-debugify passes if requested, making
    // exceptions for passes which shouldn't see -debugify instrumentation.
    bool WrapWithDebugify = DebugifyEach && !P->getAsImmutablePass() &&
                            !isIRPrintingPass(P) && !isBitcodeWriterPass(P);
    if (!WrapWithDebugify) {
      super::add(P);
      return;
    }

    // Apply -debugify/-check-debugify before/after each pass and collect
    // debug info loss statistics.
    PassKind Kind = P->getPassKind();
    StringRef Name = P->getPassName();

    // TODO: Implement Debugify for LoopPass.
    switch (Kind) {
      case PT_Function:
        super::add(createDebugifyFunctionPass());
        super::add(P);
        super::add(createCheckDebugifyFunctionPass(true, Name, &DIStatsMap));
        break;
      case PT_Module:
        super::add(createDebugifyModulePass());
        super::add(P);
        super::add(createCheckDebugifyModulePass(true, Name, &DIStatsMap));
        break;
      default:
        super::add(P);
        break;
    }
  }

  const DebugifyStatsMap &getDebugifyStatsMap() const { return DIStatsMap; }
};


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


int main_old(int argc, char** argv) {
  cout << "LLVM Normalizer for Discover" << std::endl;

  Arguments args = parseArguments(argc, argv);
  string inputFile = args.inputFile;
  string outputFile = args.outputFile;

  // process bitcode
  SMDiagnostic err;
  static LLVMContext context;

  std::unique_ptr<Module> M = parseIRFile(inputFile, err, context);

  if (printInputProgram) {
    debug() << "===============================\n"
            << "BEFORE NORMALIZATION:\n";
    M->print(debug(), nullptr);
  }

  // legacy::PassManager GPM;
  // GPM.add(new InitGlobal());

  // legacy::PassManager FPM;
  // FPM.add(new DominatorTreeWrapperPass());
  // FPM.add(new UninlineInstruction);
  // FPM.add(new CombineGEP());
  // FPM.add(new ElimIdenticalInstrs());
  // FPM.add(new ElimAllocaStoreLoad());

  // legacy::PassManager MPM;
  // MPM.add(new InitGlobal());
  // MPM.add(new ElimUnusedAuxFunction());
  // MPM.add(new InlineSimpleFunction());
  // MPM.add(new ElimUnusedGlobal());


  // Normalize all
  if (args.normalizeAll) {
    // Normalize globals first
    normalizeGlobal(*M);
    // GPM.run(*M);

    // Run each FunctionPass
    FunctionList &FS = M->getFunctionList();
    for (Function &F: FS) {
      normalizeFunction(F);
    }
    // FPM.run(*M);

    // Run ModulePass
    normalizeModule(*M);
    // MPM.run(*M);
  }
  else {
    if (!args.inlineFunction.empty()) {
      string &inlineFuncs = args.inlineFunction;

      vector<string> funcNames;
      size_t pos;
      while ((pos = inlineFuncs.find("|")) != std::string::npos) {
        funcNames.push_back(inlineFuncs.substr(0, pos));
        inlineFuncs.erase(0, pos + 1);
      }
      if (!inlineFuncs.empty())
        funcNames.push_back(inlineFuncs);

      debug() << "<<<<<<<<<<<<<<<<<<<\n"
              << "Inline functions: ";

      for (string funcName: funcNames) {
        debug() << funcName << ", ";
      }
      debug() << "\n";

      InlineSimpleFunction::inlineFunction(*M, funcNames);
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

// Use an OptionCategory to store all the flags of this tool
cl::OptionCategory DiscoverNormalizerCategory("LLVM Discover Normalizer Options",
    "Options for the LLVM-normalizer tool of the project Discover.");

static cl::opt<std::string> OutputFilename("o",
    cl::desc("Output filename"), cl::value_desc("filename"),
    cl::cat(DiscoverNormalizerCategory));

static cl::list<std::string> InputFilenames("i",
    cl::desc("Input files"), cl::value_desc("filenames"),
    cl::OneOrMore, cl::cat(DiscoverNormalizerCategory));

void setupCommandLineOptions(int argc, char** argv) {
  // reset existing options of LLVM
  // cl::ResetCommandLineParser();

  cl::HideUnrelatedOptions(DiscoverNormalizerCategory);

  cl::ParseCommandLineOptions(argc, argv, "LLVM Discover Normalizer!\n");
}

int main(int argc, char** argv) {
  // cout << "LLVM Normalizer for Discover" << std::endl;

  InitLLVM X(argc, argv);

  setupCommandLineOptions(argc, argv);

  // Enable debug stream buffering.
  // EnableDebugBuffering = true;

  LLVMContext Context;

  InitializeAllTargets();
  InitializeAllTargetMCs();
  InitializeAllAsmPrinters();
  InitializeAllAsmParsers();

  // Initialize passes
  PassRegistry &Registry = *PassRegistry::getPassRegistry();
  initializeCore(Registry);
  initializeCoroutines(Registry);
  initializeScalarOpts(Registry);
  initializeObjCARCOpts(Registry);
  initializeVectorization(Registry);
  initializeIPO(Registry);
  initializeAnalysis(Registry);
  initializeTransformUtils(Registry);
  initializeInstCombine(Registry);
  initializeAggressiveInstCombine(Registry);
  initializeInstrumentation(Registry);
  initializeTarget(Registry);

  return 0;
}
