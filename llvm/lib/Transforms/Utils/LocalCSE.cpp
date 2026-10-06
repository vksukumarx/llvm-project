//===- LocalCSE.cpp - Local common subexpression elimination --------------===//
//
// Within a basic block, an expression that was already computed is reused:
//
//   y = b * c;                tmp = b * c;
//   z = b * c + d;    =>      y = tmp; z = tmp + d;
//
//
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/Utils/LocalCSE.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include <map>
#include <tuple>

using namespace llvm;

// (opcode, icmp predicate or 0, lhs, rhs)
using ExprKey = std::tuple<unsigned, unsigned, Value *, Value *>;

static bool makeKey(Instruction &I, ExprKey &Key) {
  if (auto *BO = dyn_cast<BinaryOperator>(&I)) {
    Value *A = BO->getOperand(0), *B = BO->getOperand(1);
    // so that b*c and c*b give the same key
    if (BO->isCommutative() && B < A)
      std::swap(A, B);
    Key = ExprKey(BO->getOpcode(), 0, A, B);
    return true;
  }
  if (auto *Cmp = dyn_cast<ICmpInst>(&I)) {
    Key = ExprKey(Cmp->getOpcode(), Cmp->getPredicate(), Cmp->getOperand(0),
                  Cmp->getOperand(1));
    return true;
  }
  return false;
}

PreservedAnalyses LocalCSEPass::run(Function &F, FunctionAnalysisManager &AM) {
  errs() << "[local-cse] Function: " << F.getName() << "\n";

  bool Changed = false;
  SmallVector<Instruction *, 16> ToDelete;

  for (BasicBlock &BB : F) {
    std::map<ExprKey, Instruction *> Exprs;
    DenseMap<Value *, LoadInst *> Loads; // address -> first load

    for (Instruction &I : BB) {
      if (auto *LI = dyn_cast<LoadInst>(&I)) {
        if (!LI->isSimple())
          continue;
        Value *Addr = LI->getPointerOperand();
        LoadInst *Prev = Loads.lookup(Addr);
        if (Prev && Prev->getType() == LI->getType()) {
          errs() << "  reused" << *Prev << "  for" << *LI << "\n";
          LI->replaceAllUsesWith(Prev);
          ToDelete.push_back(LI);
          Changed = true;
        } else {
          Loads[Addr] = LI;
        }
        continue;
      }

      if (auto *SI = dyn_cast<StoreInst>(&I)) {
        Value *Addr = SI->getPointerOperand();
        if (isa<AllocaInst>(Addr))
          Loads.erase(Addr);
        else
          Loads.clear();
        continue;
      }

      if (I.mayWriteToMemory()) {
        Loads.clear();
        continue;
      }

      ExprKey Key;
      if (!makeKey(I, Key))
        continue;

      auto It = Exprs.find(Key);
      if (It == Exprs.end()) {
        Exprs[Key] = &I;
        continue;
      }

      Instruction *First = It->second;
      errs() << "  reused" << *First << "  for" << I << "\n";
      First->andIRFlags(&I); // keep only flags (nsw, ...) both copies have
      I.replaceAllUsesWith(First);
      ToDelete.push_back(&I);
      Changed = true;
    }
  }

  for (Instruction *I : ToDelete)
    I->eraseFromParent();

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
