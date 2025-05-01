#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace
{

    struct SkeletonPass : public PassInfoMixin<SkeletonPass>
    {
        PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM)
        {
            for (auto &F : M)
            {
                for (auto &B : F)
                {
                    for (auto &I : B)
                    {
                        // Check if the instruction is a store instruction
                        if (I.getOpcode() == llvm::Instruction::Store)
                        {
                            // Cast the instruction to a store instruction to execute more specific methods
                            StoreInst &SI = cast<StoreInst>(I);

                            // Get the value being stored
                            Value *val = SI.getValueOperand();

                            // Get the type of the value being stored
                            Type *typ = val->getType();

                            // Print the type to output
                            typ->print(errs());

                            // Flush the output
                            llvm::errs() << "\n";
                        }
                    }
                }
                // errs() << "I saw a function called " << F.getName() << "!\n";
            }
            return PreservedAnalyses::all();
        };
    };

}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo()
{
    return {
        .APIVersion = LLVM_PLUGIN_API_VERSION,
        .PluginName = "Skeleton pass",
        .PluginVersion = "v0.1",
        .RegisterPassBuilderCallbacks = [](PassBuilder &PB)
        {
            PB.registerPipelineStartEPCallback(
                [](ModulePassManager &MPM, OptimizationLevel Level)
                {
                    MPM.addPass(SkeletonPass());
                });
        }};
}
