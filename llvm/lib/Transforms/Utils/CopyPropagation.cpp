//===- CopyPropagation.cpp - Local copy propagation -----------------------===//
//
// A copy "y = x" at -O0 is a load from x stored into y. After it, loads of
// y in the same block are changed to load x instead:
//
//   %1 = load ptr %x                  %1 = load ptr %x
//   store %1, ptr %y          =>      store %1, ptr %y
//   %2 = load ptr %y                  %2 = load ptr %x
//
// A copy is dropped when x or y is reassigned, or on any unknown write.
//
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/Utils/CopyPropagation.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

// Var got a new value: remove "Var = x" and every "y = Var".
static void forgetVariable(DenseMap<AllocaInst *, AllocaInst *> &CopyOf,
                           AllocaInst *Var) {
  CopyOf.erase(Var);
  SmallVector<AllocaInst *, 4> Stale;
  for (auto &KV : CopyOf)
    if (KV.second == Var)
      Stale.push_back(KV.first);
  for (AllocaInst *A : Stale)
    CopyOf.erase(A);
}

static bool memoryMayChangeBetween(Instruction *From, Instruction *To) {
  for (Instruction *I = From->getNextNode(); I && I != To; I = I->getNextNode())
    if (I->mayWriteToMemory())
      return true;
  return false;
}

PreservedAnalyses CopyPropagationPass::run(Function &F,
                                           FunctionAnalysisManager &AM) {
  errs() << "[copy-propagation] Function: " << F.getName() << "\n";

  bool Changed = false;

  for (BasicBlock &BB : F) {
    // CopyOf[y] = x means y currently holds the same value as x
    DenseMap<AllocaInst *, AllocaInst *> CopyOf;

    for (Instruction &I : BB) {
      if (auto *LI = dyn_cast<LoadInst>(&I)) {
        auto *Var = dyn_cast<AllocaInst>(LI->getPointerOperand());
        if (!LI->isSimple() || !Var || !CopyOf.count(Var))
          continue;

        AllocaInst *Orig = CopyOf[Var];
        if (LI->getType() != Orig->getAllocatedType())
          continue;

        LI->setOperand(0, Orig);
        errs() << "  load of ";
        Var->printAsOperand(errs(), false);
        errs() << " now reads ";
        Orig->printAsOperand(errs(), false);
        errs() << ":" << *LI << "\n";
        Changed = true;
        continue;
      }

      if (auto *SI = dyn_cast<StoreInst>(&I)) {
        auto *Dest = dyn_cast<AllocaInst>(SI->getPointerOperand());
        if (!Dest || !SI->isSimple()) {
          CopyOf.clear();
          continue;
        }
        forgetVariable(CopyOf, Dest);

        // is this the store half of "Dest = Src"?
        auto *Ld = dyn_cast<LoadInst>(SI->getValueOperand());
        if (!Ld || !Ld->isSimple() || Ld->getParent() != &BB)
          continue;
        auto *Src = dyn_cast<AllocaInst>(Ld->getPointerOperand());
        if (!Src || Src == Dest)
          continue;
        if (Src->getAllocatedType() != Dest->getAllocatedType() ||
            Ld->getType() != Src->getAllocatedType())
          continue;
        // Src must not have changed between the load and this store
        if (memoryMayChangeBetween(Ld, SI))
          continue;

        CopyOf[Dest] = Src;
        continue;
      }

      if (I.mayWriteToMemory())
        CopyOf.clear();
    }
  }

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
