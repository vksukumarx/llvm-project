//===- ConstantPropagation.cpp - Local constant propagation ---------------===//
//
// If a variable (alloca) is assigned a constant, later loads of it in the
// same basic block are replaced by that constant.
//
//   store i32 10, ptr %a              store i32 10, ptr %a
//   %1 = load i32, ptr %a     =>      %2 = add i32 10, 5
//   %2 = add i32 %1, 5
//
// Known values are reset at the start of each block (local optimization).
//
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/Utils/ConstantPropagation.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

PreservedAnalyses ConstantPropagationPass::run(Function &F,
                                               FunctionAnalysisManager &AM) {
  errs() << "[constant-propagation] Function: " << F.getName() << "\n";

  bool Changed = false;
  SmallVector<Instruction *, 16> ToDelete;

  for (BasicBlock &BB : F) {
    DenseMap<AllocaInst *, ConstantInt *> KnownConst;

    for (Instruction &I : BB) {
      if (auto *SI = dyn_cast<StoreInst>(&I)) {
        auto *Var = dyn_cast<AllocaInst>(SI->getPointerOperand());
        if (!Var) {
          // store through an unknown pointer, could hit any variable
          KnownConst.clear();
          continue;
        }
        if (auto *C = dyn_cast<ConstantInt>(SI->getValueOperand()))
          KnownConst[Var] = C;
        else
          KnownConst.erase(Var);
        continue;
      }

      if (auto *LI = dyn_cast<LoadInst>(&I)) {
        if (!LI->isSimple())
          continue;
        auto *Var = dyn_cast<AllocaInst>(LI->getPointerOperand());
        if (!Var || !KnownConst.count(Var))
          continue;

        ConstantInt *C = KnownConst[Var];
        if (C->getType() != LI->getType())
          continue;

        errs() << "  replaced" << *LI << "  with constant " << *C << "\n";
        LI->replaceAllUsesWith(C);
        ToDelete.push_back(LI);
        Changed = true;
        continue;
      }

      // calls etc. may change any variable
      if (I.mayWriteToMemory())
        KnownConst.clear();
    }
  }

  // erase after the loop so we don't break the iteration
  for (Instruction *I : ToDelete)
    I->eraseFromParent();

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
