#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"
#include "llvm/Transforms/Utils/ScalarEvolutionExpander.h"
#include "llvm/IR/CFG.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Analysis/DominanceFrontier.h"
#include <vector>
#include <memory>
#include <algorithm>


using namespace llvm;

namespace
{

    struct MyDomAnalysis : public AnalysisInfoMixin<MyDomAnalysis>
    {

        struct Result {
            // Declare the map to hold our block -> list of dominators data
            DenseMap<BasicBlock*, unsigned> BlockIndices;

            std::vector<BasicBlock*> Blocks;

            // For each block, a bitvector of size N: bit i is true
            // if Blocks[i] dominates this block
             std::vector<BitVector> DomSets;

            // Declare list of immediate dominators
            std::vector<unsigned> idoms; // (blockIdx, domIdx)

            // Declare list of children of each block (the blocks that it is the immediate dominator of)
            std::vector<std::vector<unsigned>> Children;

            // Declare the dominance frontier
            std::vector<std::vector<unsigned>> DominanceFrontier;

            // Helper function to compute the dominance frontier
            void computeDF(unsigned idx){
                // For each child of 'idx', compute its dominance frontier
                for (unsigned childIdx: Children[idx]){
                    computeDF(childIdx);

                    // Go through each block in the child's dominance frontier
                    for (unsigned frontierBlockIdx: DominanceFrontier[childIdx]){
                        // If this block is not immediately dominated by 'idx', then add to 'idx's dominance frontier
                        if (idoms[frontierBlockIdx] != idx){
                            DominanceFrontier[idx].push_back(frontierBlockIdx);
                        }
                        
                    }

                }
                
            }

            // Constructor for our Result struct
            Result(Function &F){
                // Orchestrates the steps in here
                // Figure out how many basic blocks we have and assign each of them an index (to be used later)
                int idx = 0;
                for (BasicBlock &BB: F){
                    BlockIndices[&BB] = idx;
                    Blocks.push_back(&BB);
                    idx = idx + 1;
                }

                unsigned N = Blocks.size();


                // Initialize the dense map so that each block is dominated by every other block (aside from initial block)
                for (int idx = 0; idx < N; idx = idx + 1){
                    DomSets.emplace_back(BitVector(N, true));
                }


                // However, the entry block should only dominate itself
                unsigned entryIdx = BlockIndices[&F.getEntryBlock()];
                DomSets[entryIdx].reset();
                DomSets[entryIdx].set(entryIdx);

                // Refine until convergence
                bool changed = true;

                while (changed) {
                    changed = false;

                    // Iterate through all the blocks
                    for (BasicBlock *BB: Blocks){
                    
                        // If this is the entry block, skip it
                        if (BB == &F.getEntryBlock()){
                            continue;
                        }

                        // Get the block's index
                        int idx = BlockIndices[BB];

                        // Initialize its new bitvector
                        BitVector newDom = BitVector(N, true);

                        // Take the intersection of all the dominators of this block's predecessors
                        for (BasicBlock *pred: predecessors(BB)){

                            // Get our index representation for the predecessor
                            int pred_idx = BlockIndices[pred];

                            // Get the predecessor's BitVector of doms
                            const BitVector &pred_doms = DomSets[pred_idx];

                            // Take the intersection
                            newDom &= pred_doms;
                        }

                        // Add back this block to its own dom set
                        newDom.set(idx);


                        // Check if its dom set changed
                        if (DomSets[idx] != newDom){
                            changed = true;
                            DomSets[idx] = newDom;
                        }

                    }
                }


                //----------------------------------------------//
                // Form dominator tree

                idoms.resize(N);
                Children.resize(N);


                // Iterate through the list of blocks
                for (BasicBlock *BB: Blocks){

                    // Get this block's index representation
                    unsigned idx = BlockIndices[BB];

                    // Skip the entry block
                    if (idx == entryIdx){
                        continue;
                    }

                    // Get this block's list of dominators
                    BitVector domSet = DomSets[idx];

                    // Remove this block from its list of dominators (to get list of strict dominators)
                    domSet.reset(idx);

                    // Find the dominator in its list of dominators that does not dominate any of the other dominators
                    unsigned bestIdx = entryIdx;

                    // Loop through each dominator of this block and check grab the one with the most dominators
                    for (BasicBlock *dom: Blocks){
                        unsigned domIdx = BlockIndices[dom];

                        // First check that this block actually dominates our current block
                        if (domSet[domIdx] == 1){
                            // Grab this dominator's list of dominators
                            BitVector domsDomSet = DomSets[domIdx];


                            // Determine which one has the most dominators
                            if (domsDomSet.count() > DomSets[bestIdx].count()){
                                bestIdx = domIdx;
                            }
                        }
                    }

                    idoms[idx] = bestIdx;
                
                    // Add this block as a child of dom
                    Children[bestIdx].push_back(idx);
                }

                //----------------------------------------------//
                // Determine the dominance frontier using Cytron's alg.

                DominanceFrontier.assign(N, {});

                
                // Loop through each basic block
                for (BasicBlock *BB: Blocks){
                    unsigned blockIdx = BlockIndices[BB];

                    // Compute part of its dominance frontier using its successors
                    for (BasicBlock *succ: successors(BB)){
                        unsigned succIdx = BlockIndices[succ];

                        // If the current block does not immediately dominate this successor, then add this successor to DF
                        if (idoms[succIdx] != blockIdx){
                            DominanceFrontier[blockIdx].push_back(succIdx);
                        }
                    }
                }

                // Recursively build up each block's dominance frontier
                computeDF(entryIdx);

                // Deduplicate each block's frontier list
                for (auto &DFb : DominanceFrontier) {
                // Sort the indices so that duplicates become adjacent
                std::sort(DFb.begin(), DFb.end());
                // Erase the duplicates
                DFb.erase(std::unique(DFb.begin(), DFb.end()), DFb.end());
                }



                errs() << "=== Dominator Analysis Results ===\n";

                // 1) Print the Blocks vector
                errs() << "Blocks (" << Blocks.size() << "):\n";
                for (auto *BB : Blocks) {
                errs() << "  [" << BlockIndices[BB] << "] " << BB->getName() << "\n";
                }

                // 2) Print the BlockIndex map
                errs() << "BlockIndex map:\n";
                for (auto &KV : BlockIndices) {
                errs() << "  " << KV.first->getName() << " -> " << KV.second << "\n";
                }

                // 3) Print the DomSets
                errs() << "DomSets (dominator lists):\n";
                for (unsigned i = 0; i < Blocks.size(); ++i) {
                BasicBlock *BB = Blocks[i];
                unsigned idx = BlockIndices[BB];
                errs() << "  " << idx<< " dominated by: ";
                bool first = true;
                for (unsigned j = 0; j < Blocks.size(); ++j) {
                    if (DomSets[i].test(j)) {
                    if (!first) errs() << ", ";
                    BasicBlock *BB = Blocks[j];
                    errs() << BlockIndices[BB];
                    first = false;
                    }
                }
                errs() << "\n";
                }

                errs() << "===============================\n";

                errs() << "--- Immediate Dominators ---\n";
                for (unsigned i = 0; i < N; ++i) {
                errs() << "[" << i << "] idom = ";
                if (i == entryIdx)
                    errs() << "none\n";
                else
                    errs() << idoms[i] << "\n";
                }

                errs() << "--- Dominator Tree (Children) ---\n";
                for (unsigned i = 0; i < N; ++i) {
                errs() << "[" << i << "] children:";
                for (unsigned c : Children[i])
                    errs() << " " << c;
                errs() << "\n";
                }

                errs() << "--- Dominance Frontier ---\n";
                for (unsigned i = 0; i < N; ++i) {
                errs() << "[" << i << "] DF:";
                for (unsigned w : DominanceFrontier[i])
                    errs() << " " << w;
                errs() << "\n";
                }

                errs() << "===============================\n";

            
            }

        };


    Result run(Function &F, FunctionAnalysisManager &AM){
        // Compute and register the result for F
        //AM.registerPass<MyDomAnalysis>(F, Result(F)); don't need to register right now
        return Result(F);
    }

    // Required for AnalysisInfoMixin
    static llvm::AnalysisKey Key;

    // Allow access to Result type
   friend struct AnalysisInfoMixin<MyDomAnalysis>;

    };

    // Define the analysis key
    AnalysisKey MyDomAnalysis::Key;



    // Printer pass to consume MyDomAnalysis
    struct MyDomPrinter : public PassInfoMixin<MyDomPrinter> {
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM) {
        // Trigger the analysis, which prints results in its Result constructor
        FAM.getResult<MyDomAnalysis>(F);
        errs() << "runninnngng";
        return PreservedAnalyses::all();
  }
};

   struct LlvmDomPrinter : PassInfoMixin<LlvmDomPrinter> {
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM) {
    // --- Build block → index mapping ---
    std::vector<BasicBlock*> Blocks;
    DenseMap<BasicBlock*, unsigned> Idx;
    unsigned counter = 0;
    for (BasicBlock &BB : F) {
      Idx[&BB] = counter;
      Blocks.push_back(&BB);
      ++counter;
    }
    unsigned N        = Blocks.size();
    unsigned entryIdx = Idx[&F.getEntryBlock()];

    // --- Get LLVM's DominatorTree and DominanceFrontier ---
    auto &DT = AM.getResult<DominatorTreeAnalysis>(F);
    auto &DF = AM.getResult<DominanceFrontierAnalysis>(F);

    // --- Print full DomSets ---
    errs() << "DomSets (dominator lists):\n";
    for (unsigned i = 0; i < N; ++i) {
      errs() << "  [" << i << "] dominated by:";
      for (unsigned j = 0; j < N; ++j) {
        if (DT.dominates(Blocks[j], Blocks[i]))
          errs() << " " << j;
      }
      errs() << "\n";
    }

    // --- Print immediate dominators ---
    errs() << "--- Immediate Dominators ---\n";
    for (unsigned i = 0; i < N; ++i) {
      BasicBlock *BB = Blocks[i];
      auto *Node     = DT.getNode(BB);
      errs() << "[" << i << "] idom = ";
      if (i == entryIdx || !Node->getIDom())
        errs() << "none\n";
      else {
        unsigned p = Idx[ Node->getIDom()->getBlock() ];
        errs() << p << "\n";
      }
    }

    // --- Print Dominator Tree (children) ---
    errs() << "--- Dominator Tree (Children) ---\n";
    // Build children lists:
    std::vector<std::vector<unsigned>> Children(N);
    for (unsigned i = 0; i < N; ++i) {
      BasicBlock *BB = Blocks[i];
      auto *Node     = DT.getNode(BB);
      if (Node->getIDom()) {
        unsigned p = Idx[ Node->getIDom()->getBlock() ];
        Children[p].push_back(i);
      }
    }
    for (unsigned i = 0; i < N; ++i) {
      errs() << "[" << i << "] children:";
      for (unsigned c : Children[i])
        errs() << " " << c;
      errs() << "\n";
    }

  // --- Dominance Frontier ---  
  errs() << "--- Dominance Frontier ---\n";
  for (unsigned i = 0; i < Blocks.size(); ++i) {
    BasicBlock *BB = Blocks[i];
    errs() << "[" << i << "] DF:";
    // Use find() to look up BB’s entry in the map
    auto it = DF.find(BB);                                 // :contentReference[oaicite:0]{index=0}
    if (it != DF.end()) {
      for (BasicBlock *S : it->second) {
        errs() << " " << Idx[S];
      }
    }
    errs() << "\n";
  }
  errs() << "========================================\n\n";
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
            PB.registerAnalysisRegistrationCallback(
            [](FunctionAnalysisManager &FAM) {
              FAM.registerPass([]() { return MyDomAnalysis(); });
            });


        // Register the printer pass for -passes="my-dom-analysis"
        PB.registerPipelineParsingCallback(
            [](StringRef Name, FunctionPassManager &FPM,
               ArrayRef<PassBuilder::PipelineElement>) {
              if (Name == "my-dom-analysis") {
                FPM.addPass(MyDomPrinter());
                return true;
              }
              return false;
            });

        // Register the printer pass for LLVM's dom analysis
        PB.registerPipelineParsingCallback(
            [](StringRef Name, FunctionPassManager &FPM,
               ArrayRef<PassBuilder::PipelineElement>) {
              if (Name == "print-llvm-dom") {
                FPM.addPass(LlvmDomPrinter());
                return true;
              }
              return false;
            });
      }};
}
