//===- SimpleDCE.cpp - Simple dead code elimination -----------------------===//
//
// Removes code that never runs or whose result is never used:
//   1. "br i1 <const>" becomes an unconditional branch
//   2. blocks not reachable from the entry block are deleted
//   3. variables that are only stored to (never loaded) are deleted
//   4. instructions with no uses and no side effects are deleted
//
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/Utils/SimpleDCE.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

static void printBlock(BasicBlock *BB) {
  BB->printAsOperand(errs(), false);
}

static bool foldConstantBranches(Function &F) {
  bool Changed = false;
  for (BasicBlock &BB : F) {
    auto *Br = dyn_cast<CondBrInst>(BB.getTerminator());
    if (!Br)
      continue;
    auto *Cond = dyn_cast<ConstantInt>(Br->getCondition());
    if (!Cond)
      continue;

    BasicBlock *Taken = Br->getSuccessor(Cond->isOne() ? 0 : 1);
    BasicBlock *NotTaken = Br->getSuccessor(Cond->isOne() ? 1 : 0);

    errs() << "  branch on constant " << *Cond << " in ";
    printBlock(&BB);
    errs() << " now always goes to ";
    printBlock(Taken);
    errs() << "\n";

    if (NotTaken != Taken)
      NotTaken->removePredecessor(&BB); // fixes up phis in NotTaken
    UncondBrInst::Create(Taken, Br->getIterator());
    Br->eraseFromParent();
    Changed = true;
  }
  return Changed;
}

static bool removeUnreachableBlocks(Function &F) {
  SmallPtrSet<BasicBlock *, 32> Reachable;
  SmallVector<BasicBlock *, 32> Worklist;
  Worklist.push_back(&F.getEntryBlock());
  Reachable.insert(&F.getEntryBlock());

  while (!Worklist.empty()) {
    BasicBlock *BB = Worklist.pop_back_val();
    for (BasicBlock *Succ : successors(BB))
      if (Reachable.insert(Succ).second)
        Worklist.push_back(Succ);
  }

  SmallVector<BasicBlock *, 8> Dead;
  for (BasicBlock &BB : F)
    if (!Reachable.count(&BB))
      Dead.push_back(&BB);
  if (Dead.empty())
    return false;

  for (BasicBlock *BB : Dead) {
    errs() << "  removing unreachable block ";
    printBlock(BB);
    errs() << "\n";
    for (BasicBlock *Succ : successors(BB))
      if (Reachable.count(Succ))
        Succ->removePredecessor(BB);
  }

  // dead blocks can refer to each other, so drop references before erasing
  for (BasicBlock *BB : Dead)
    BB->dropAllReferences();
  for (BasicBlock *BB : Dead)
    BB->eraseFromParent();
  return true;
}

static bool removeWriteOnlyVariables(Function &F) {
  SmallVector<AllocaInst *, 8> Vars;

  for (BasicBlock &BB : F) {
    for (Instruction &I : BB) {
      auto *Var = dyn_cast<AllocaInst>(&I);
      if (!Var)
        continue;
      bool OnlyStores = true;
      for (User *U : Var->users()) {
        auto *SI = dyn_cast<StoreInst>(U);
        if (!SI || SI->getPointerOperand() != Var || !SI->isSimple()) {
          OnlyStores = false;
          break;
        }
      }
      if (OnlyStores)
        Vars.push_back(Var);
    }
  }

  for (AllocaInst *Var : Vars) {
    errs() << "  removing write-only variable" << *Var << "\n";
    SmallVector<User *, 8> Stores(Var->users());
    for (User *U : Stores)
      cast<Instruction>(U)->eraseFromParent();
    Var->eraseFromParent();
  }
  return !Vars.empty();
}

static bool removeUnusedInstructions(Function &F) {
  bool Changed = false;
  // removing one instruction can leave its operands unused, so loop
  while (true) {
    SmallVector<Instruction *, 16> Dead;
    for (BasicBlock &BB : F)
      for (Instruction &I : BB)
        if (!I.isTerminator() && !I.isEHPad() && I.use_empty() &&
            !I.mayHaveSideEffects())
          Dead.push_back(&I);

    if (Dead.empty())
      break;
    for (Instruction *I : Dead) {
      errs() << "  removing unused" << *I << "\n";
      I->eraseFromParent();
    }
    Changed = true;
  }
  return Changed;
}

PreservedAnalyses SimpleDCEPass::run(Function &F, FunctionAnalysisManager &AM) {
  errs() << "[simple-dce] Function: " << F.getName() << "\n";

  bool Changed = false;
  Changed |= foldConstantBranches(F);
  Changed |= removeUnreachableBlocks(F);
  Changed |= removeWriteOnlyVariables(F);
  Changed |= removeUnusedInstructions(F);

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
