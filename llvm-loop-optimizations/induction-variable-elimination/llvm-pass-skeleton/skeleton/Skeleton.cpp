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

    struct IVEPass : public PassInfoMixin<IVEPass>
    {
        static StringRef name() { return "ive-pass"; }
        PreservedAnalyses run(Loop &L, LoopAnalysisManager &LAM, LoopStandardAnalysisResults &AR,
                              LPMUpdater &U)
        {

            // Grab everything I need to perform IVE
            ScalarEvolution &SE = AR.SE;

            // Ensure loop is in a valid state
            BasicBlock *Preheader = L.getLoopPreheader();
            BasicBlock *Header = L.getHeader();
            BasicBlock *Latch = L.getLoopLatch();
            if (!Preheader || !Header || !Latch)
            {
                errs() << "Missing preheader, header, or latch\n";
                return PreservedAnalyses::all();
            }



            struct IVJob
            {
                const SCEVAddRecExpr *Expr;
                Instruction *OldInst;
            };

            // Initialize a list to hold all the induction variables for this loop
            SmallVector<IVJob, 8> Jobs;
            SmallVector<Instruction *, 8> ToErase;

            // Loop through each basic block in the loop
            for (BasicBlock *B : L.blocks())
            {

                // Loop through each instruction
                for (Instruction &I : *B)
                {
                    const SCEV *S = SE.getSCEV(&I);

                    // Check if this instruction is an add-recurrence
                    if (auto *ARExpr = dyn_cast<SCEVAddRecExpr>(S))
                    {
                        errs() << "An SCEVAddRecExpr: \n";
                        I.print(errs());
                        errs() << "\n\n";
                        // Only add affine expressions that belong to this loop to our list
                        if (ARExpr->getLoop() == &L && ARExpr->isAffine())
                        {
                            Type *Ty = ARExpr->getType();
                            if (Ty->isPointerTy())
                            { // These are currently the only supported types so don't erase anything else

                                Jobs.push_back({ARExpr, &I});
                            }
                        }
                    }
                }
            }


            Instruction *PreTerm = Preheader->getTerminator();
            IRBuilder<> PreHeaderBuilder(PreTerm);

            IRBuilder<> HeaderBuilder(&*Header->getFirstInsertionPt());

            Instruction *LatchTerm = Latch->getTerminator();
            IRBuilder<> LatchBuilder(LatchTerm);

            Module &M = *Latch->getModule();
            const DataLayout &DL = M.getDataLayout();

            // Initialize the expander
            SCEVExpander Expander(SE, DL, "ive");

            for (auto &J : Jobs)
            {

                // Unpack the struct
                const SCEVAddRecExpr *ARExpr = J.Expr;
                Instruction *Old = J.OldInst;

                // Get start value to use for constructing Pre-Header
                Value *StartVal = Expander.expandCodeFor(
                    ARExpr->getStart(),
                    ARExpr->getType(),
                    PreTerm);

                PHINode *NewPhi = nullptr;

                // Form replacement instructions
                Value *NextVal;
                Type *Ty = ARExpr->getType();
                if (Ty->isPointerTy())
                {
                    StartVal->getType()->print(errs());


                    // strip off any bitcast/intrin‐casts so you get to the true source
                    Value *Base = StartVal->stripPointerCasts();


                    // 2) peel away any of the three GEP flavors
                    if (auto *CE = dyn_cast<ConstantExpr>(Base)) {
                        if (CE->getOpcode() == Instruction::GetElementPtr)
                        Base = CE->getOperand(0)->stripPointerCasts();
                    }
                    else if (auto *GO = dyn_cast<GEPOperator>(Base)) {
                        // covers both ConstantExpr and Instruction GEP operators
                        Base = GO->getPointerOperand()->stripPointerCasts();
                    }
                    else if (auto *GI = dyn_cast<GetElementPtrInst>(Base)) {
                        Base = GI->getPointerOperand()->stripPointerCasts();
                    }
  
                    ArrayType *ArrTy = nullptr;
                    Type *EltTy = nullptr;
                    // 2) If it’s a global, grab its array type
                    if (auto *GV = dyn_cast<GlobalVariable>(Base)) {
                        // getValueType() on a GlobalVariable returns the IR‐type of the global itself
                        if (auto *AT = dyn_cast<ArrayType>(GV->getValueType())) {
                        ArrTy = AT;
                        EltTy = ArrTy->getElementType();  // this is i32
                        unsigned NumElements = ArrTy->getNumElements();

                        }
                    }

                    // Handle non-global arrays
                    if (auto *AI = dyn_cast<AllocaInst>(Base))
                    {
                        if (auto *AT = dyn_cast<ArrayType>(AI->getAllocatedType()))
                        {
                            ArrTy = AT;
                            EltTy = ArrTy->getElementType();  // this is i32
                        
                        }
                    }
                      // skip this Job entirely and leave the loop’s IR untouched.
                        if (!ArrTy || !EltTy) {
                        errs() << "IVEPass: couldn’t infer ArrayType/ElementType—skipping this induction\n";
                        continue;
  }
                    // Declare the phi node for this inductive variable
                    NewPhi = HeaderBuilder.CreatePHI(
                        ARExpr->getType(),
                        2,
                        "ive.pointer.phi");

                    auto *OldGEP = cast<GetElementPtrInst>(Old);
                    Value *BasePtr = OldGEP->getPointerOperand()->stripPointerCasts();




                    Value *IdxVal = OldGEP->getOperand(OldGEP->getNumOperands() - 1);

                    // Make sure it is the phi node:
                    auto *IdxPhi = cast<PHINode>(IdxVal);

                    Value *StartIdx = IdxPhi->getIncomingValueForBlock(Preheader);


                    if (!ArrTy) {
                        errs() << "ERROR: ArrTy is null — failed to infer ArrayType!\n";
                      } else {
                        errs() << "Got ArrTy = ["
                               << ArrTy->getNumElements()
                               << " x ";
                        ArrTy->getElementType()->print(errs());
                        errs() << "]\n";
                      }
                      
                      if (!EltTy) {
                        errs() << "ERROR: EltTy is null — cannot extract element type!\n";
                      } else {
                        errs() << "Got EltTy = ";
                        EltTy->print(errs());
                        errs() << "\n";
                      }

                    // Build {0, IdxPhi} so we pick element IdxPhi
                    Value *Zero = ConstantInt::get(IdxPhi->getType(), 0);
                    SmallVector<Value *, 2> Idxs{Zero, StartIdx};

                    // Put initial pointer creator in Preheader
                    Value *InitialPointer = PreHeaderBuilder.CreateInBoundsGEP(ArrTy, BasePtr, Idxs, "ive.initial.pointer");

                    NewPhi->addIncoming(InitialPointer, Preheader);



                    // Figure out the target integer type for pointers on this target:
                    LLVMContext &Ctx = LatchBuilder.getContext();
                    Type *IntPtrTy = DL.getIntPtrType(Ctx); // from your DataLayout DL

                    // 4) Compute the byte‐stride constant = sizeof(EltTy)
                    EltTy->print(errs());
                    uint64_t Bytes = DL.getTypeAllocSize(EltTy);

                    
                    Value *StepConst = ConstantInt::get(
                        DL.getIntPtrType(Ctx),
                        Bytes
                    );

                    Value *StepVal = Expander.expandCodeFor(
                        ARExpr->getStepRecurrence(SE),  // still the same SCEV
                        IntPtrTy,                       
                        LatchTerm
                    );


                    // Insert ptrtoint
                    Value *PtrAsInt = LatchBuilder.CreatePtrToInt(NewPhi, IntPtrTy, "ive.ptrtoi");
                    InitialPointer->getType()->print(errs());

                    // // Insert add stride
                    Value *NewInt = LatchBuilder.CreateAdd(PtrAsInt, StepVal, "ive.add");

                    // turn it back into a pointer of the same type as original Ptr:
                    Value *NextVal = LatchBuilder.CreateIntToPtr(NewInt, InitialPointer->getType(), "ive.next");


                    // Add this incremented induction variable to our Phi node (coming from the latch block)
                    NewPhi->addIncoming(NextVal, Latch);


                    Old->replaceAllUsesWith(NewPhi); // replace all the old instructions with our new phi node


                    // Make sure to delete old instruction
                    ToErase.push_back(Old);
                    }



                }
            // Delete the old instructions
            for (Instruction *Old : ToErase)
            {
                Old->print(errs());
                errs() << "\n\n";
                Old->eraseFromParent();
            }
            

            return PreservedAnalyses::none();
        };
    };

};


extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo()
{
    return {
        .APIVersion = LLVM_PLUGIN_API_VERSION,
        .PluginName = "IVEpass",
        .PluginVersion = "v0.1",
        .RegisterPassBuilderCallbacks = [](PassBuilder &PB)
        {
            // 1) still inject at end of default loop pipeline if you do -O2…
            PB.registerLoopOptimizerEndEPCallback(
                [](LoopPassManager &LPM, OptimizationLevel)
                {
                    LPM.addPass(IVEPass());
                });

            // 2) teach 'opt -passes="loop(ive-pass)"' about our pass:
            PB.registerPipelineParsingCallback(
                [](StringRef Name,
                   LoopPassManager &LPM,
                   ArrayRef<PassBuilder::PipelineElement> /*unused*/)
                {
                    if (Name == "ive-pass")
                    {
                        LPM.addPass(IVEPass());
                        return true; // we handled it
                    }
                    return false; // not ours, let other callbacks try
                });
        }};
}
