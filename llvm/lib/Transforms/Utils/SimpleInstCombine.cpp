//===- SimpleInstCombine.cpp - Simple instruction combining ---------------===//
//
// Strength reduction on multiply / unsigned divide by a constant:
//   x * 0 -> 0,   x * 1 -> x,   x * 2^k -> x << k
//   x / 1 -> x,   x / 2^k -> x >> k   (udiv only)
//
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/Utils/SimpleInstCombine.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

// Returns the replacement value, or nullptr. New shifts go before BO.
static Value *combine(BinaryOperator *BO) {
  Type *Ty = BO->getType();
  Value *X = BO->getOperand(0);
  auto *C = dyn_cast<ConstantInt>(BO->getOperand(1));

  if (BO->getOpcode() == Instruction::Mul) {
    if (!C) { // constant may be on the left: 8 * x
      X = BO->getOperand(1);
      C = dyn_cast<ConstantInt>(BO->getOperand(0));
    }
    if (!C)
      return nullptr;
    const APInt &V = C->getValue();
    if (V.isZero())
      return ConstantInt::get(Ty, 0);
    if (V.isOne())
      return X;
    if (V.isPowerOf2())
      return BinaryOperator::CreateShl(X, ConstantInt::get(Ty, V.logBase2()),
                                       "", BO->getIterator());
    return nullptr;
  }

  if (BO->getOpcode() == Instruction::UDiv && C) {
    const APInt &V = C->getValue();
    if (V.isOne())
      return X;
    if (V.isPowerOf2())
      return BinaryOperator::CreateLShr(X, ConstantInt::get(Ty, V.logBase2()),
                                        "", BO->getIterator());
  }
  return nullptr;
}

PreservedAnalyses SimpleInstCombinePass::run(Function &F,
                                             FunctionAnalysisManager &AM) {
  errs() << "[simple-instcombine] Function: " << F.getName() << "\n";

  bool Changed = false;
  SmallVector<Instruction *, 16> ToDelete;

  for (BasicBlock &BB : F) {
    for (Instruction &I : BB) {
      auto *BO = dyn_cast<BinaryOperator>(&I);
      if (!BO)
        continue;
      Value *New = combine(BO);
      if (!New)
        continue;

      errs() << "  replaced" << *BO << "  with " << *New << "\n";
      BO->replaceAllUsesWith(New);
      ToDelete.push_back(BO);
      Changed = true;
    }
  }

  for (Instruction *I : ToDelete)
    I->eraseFromParent();

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
