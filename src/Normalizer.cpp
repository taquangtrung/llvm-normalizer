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
#include "llvm/Support/PrettyStackTrace.h"


#include "llvm/Transforms/Utils/Debugify.h"

// #include "llvm/Transforms/AggressiveInstCombine/AggressiveInstCombine.h"

#include "Debug.h"
#include "InitGlobal.h"
#include "UninlineInstruction.h"
#include "InlineSimpleFunction.h"
#include "ElimUnusedAuxFunction.h"
#include "ElimUnusedGlobal.h"
#include "ElimIdenticalInstrs.h"
#include "CombineGEP.h"
#include "ElimAllocaStoreArg.h"

using namespace std;
using namespace llvm;
using namespace discover;

typedef struct Arguments {
  string inputFile;
  string outputFile;
  bool normalizeAll;
  string inlineFunction;
} Arguments;

static cl::opt<bool> DebugifyEach(
    "debugify-each",
    cl::desc("Start each pass with debugify and end it with check-debugify"));

// Customized Pass Manager
class NormalizerPassManager : public legacy::PassManager {
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

// Declare command line options

// Use an OptionCategory to store all the flags of this tool
cl::OptionCategory DiscoverNormalizerCategory("LLVM Discover Normalizer Options",
                                              "Options for the LLVM-normalizer tool of the project Discover.");

static cl::opt<string> InputFilename(cl::Positional,
                                     cl::desc("<Input bitcode file>"),
                                     cl::init("-"), cl::value_desc("filename"));

static cl::opt<string> OutputFilename("o",
                                      cl::desc("<Output bitcode file>"),
                                      cl::value_desc("filename"),
                                      cl::cat(DiscoverNormalizerCategory));

static cl::opt<bool> NoVerify("disable-verify",
                              cl::desc("Do not run the verifier"), cl::Hidden);

static cl::opt<bool> VerifyEach("verify-each",
                                cl::desc("Verify after each transform"));

static cl::opt<bool> VerifyOnly("verify-only",
                                cl::desc("Only verify, not transform bitcode"));

static cl::opt<bool> DisableInline("disable-inlining",
                                   cl::desc("Do not run the inliner pass"));

static cl::opt<bool> Debugging("debug", cl::desc("Enable debugging"),
                               cl::cat(DiscoverNormalizerCategory));

static cl::opt<bool> PrintInputProgram("pip",
                                       cl::desc("Enable printing input program"),
                                       cl::cat(DiscoverNormalizerCategory));

static cl::opt<bool> PrintOutputProgram("pop",
                                        cl::desc("Enable printing input program"),
                                        cl::cat(DiscoverNormalizerCategory));

static cl::opt<bool> PrintOutputEach("print-output-each",
                                     cl::desc("Print output program after each pass"),
                                     cl::cat(DiscoverNormalizerCategory));

//------------------------------

static inline void addModulePass(legacy::PassManagerBase &PM, ModulePass *P) {
  // Add the pass to the pass manager...
  PM.add(P);

  // If we are verifying all of the intermediate steps, add the verifier...
  if (VerifyEach)
    PM.add(createVerifierPass());

  if (PrintOutputEach)
    PM.add(createPrintModulePass(outs()));
}


static inline void addFunctionPass(legacy::PassManagerBase &PM, FunctionPass *P) {
  // Add the pass to the pass manager...
  PM.add(P);

  // If we are verifying all of the intermediate steps, add the verifier...
  if (VerifyEach)
    PM.add(createVerifierPass());

  if (PrintOutputEach)
    PM.add(createPrintFunctionPass(outs()));
}


//----------------------------------

int main(int argc, char** argv) {
  debug() << "LLVM Normalizer for Discover\n";

  InitLLVM X(argc, argv);

  // Parse command line options
  cl::HideUnrelatedOptions(DiscoverNormalizerCategory);
  cl::ParseCommandLineOptions(argc, argv, "LLVM Discover Normalizer!\n");

  // Retrieve flags from CLI
  debugging = Debugging;
  printInputProgram = PrintInputProgram;
  printOutputProgram = PrintOutputProgram;

  // Load the input module...
  LLVMContext Context;
  SMDiagnostic Err;
  std::unique_ptr<Module> M = parseIRFile(InputFilename, Err, Context);

  if (printInputProgram) {
    debug() << "===========================================\n"
            << "Input Bitcode Program: \n\n";
    M->print (debug(), nullptr);
    debug() << "\n";
  }

  // Initialization
  InitializeAllTargets();

  // Create module and function analysis pass manager
  NormalizerPassManager ModulePasses;
  std::unique_ptr<legacy::FunctionPassManager> FuncPasses;
  FuncPasses.reset(new legacy::FunctionPassManager(M.get()));

  if (VerifyOnly)
    ModulePasses.add(createVerifierPass());

  if (!VerifyOnly) {
    // Add module passes
    addModulePass(ModulePasses, new InitGlobal());
    addModulePass(ModulePasses, new ElimUnusedAuxFunction());
    addModulePass(ModulePasses, new InlineSimpleFunction());
    addModulePass(ModulePasses, new ElimUnusedGlobal());

    // Add function passes
    addFunctionPass(*FuncPasses, new ElimAllocaStoreArg());
    addFunctionPass(*FuncPasses, new UninlineInstruction());
    addFunctionPass(*FuncPasses, new CombineGEP());
    addFunctionPass(*FuncPasses, new ElimIdenticalInstrs());
  }

  // Run module passes
  ModulePasses.run(*M);

  // Run function passes
  FuncPasses->doInitialization();
  for (Function &F: *M) {
    FuncPasses->run(F);
  }
  FuncPasses->doFinalization();

  if (printOutputProgram) {
    debug() << "===========================================\n"
            << "Output Bitcode Program: \n\n";
    M->print (debug(), nullptr);
    debug() << "\n";
  }

  // write output
  if (!OutputFilename.empty()) {
    std::error_code EC;
    raw_fd_ostream OS(OutputFilename, EC, llvm::sys::fs::F_None);
    WriteBitcodeToFile(*M, OS);
    OS.flush();
  }

  return 0;
}
