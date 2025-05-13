#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"
#include "llvm/Transforms/Utils/ScalarEvolutionExpander.h"

using namespace llvm;

namespace
{

    struct MyDomAnalysis : public PassInfoMixin<MyDomAnalysis>
    {

        struct Result {
            

            // Constructor for our Result struct
            Result(Function &F){
                // Orchestrates the steps in here
            }

            // data fiels and helper functions
        }


    PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM){
        // Compute and register the result for F
        AM.registerPassResult<MyDomAnalysis>(F, Result(F));
        return PreservedAnalyses::all();
    }

    };

}


extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo()
{
    return {
        .APIVersion = LLVM_PLUGIN_API_VERSION,
        .PluginName = "my-dom-analysis",
        .PluginVersion = "v0.1",
        .RegisterPassBuilderCallbacks = [](PassBuilder &PB)
        {
            // still inject at end of default loop pipeline if you do -O2…
            PB.registerScalarOptimizerLateEPCallback(
                [](FunctionPassManager &FPM, OptimizationLevel)
                {
                    FPM.addPass(MyDomAnalysis());
                });

            // teach `opt -passes="func(my-dom-analysis)"` about our pass
            PB.registerPipelineParsingCallback(
                [](StringRef Name,
                   FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement> /*unused*/)
                {
                    if (Name == "my-dom-analysis")
                    {
                        FPM.addPass(MyDomAnalysis());
                        return true; // we handled it
                    }
                    return false; // not ours, let other callbacks try
                });
        }};
}
