//===- ConstantFoldingPass.cpp - Local constant folding -------------------===//
//
// Evaluates binary operators and integer compares whose operands are all
// constants, e.g. "mul i32 2, 3" -> 6. Results are substituted into later
// instructions, so a chain like 2 * 3 + 4 folds to 10 in one walk.
//
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/Utils/ConstantFoldingPass.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

// Computes L op R. Returns false when the result is undefined
// (divide by zero, INT_MIN / -1, shift >= bit width) or op is not handled.
static bool foldBinary(unsigned Opcode, const APInt &L, const APInt &R,
                       APInt &Result) {
  bool SignedOverflow = L.isMinSignedValue() && R.isAllOnes();
  bool BadShift = R.uge(L.getBitWidth());

  switch (Opcode) {
  case Instruction::Add:  Result = L + R; return true;
  case Instruction::Sub:  Result = L - R; return true;
  case Instruction::Mul:  Result = L * R; return true;
  case Instruction::And:  Result = L & R; return true;
  case Instruction::Or:   Result = L | R; return true;
  case Instruction::Xor:  Result = L ^ R; return true;

  case Instruction::SDiv:
    if (R.isZero() || SignedOverflow)
      return false;
    Result = L.sdiv(R);
    return true;
  case Instruction::SRem:
    if (R.isZero() || SignedOverflow)
      return false;
    Result = L.srem(R);
    return true;
  case Instruction::UDiv:
    if (R.isZero())
      return false;
    Result = L.udiv(R);
    return true;
  case Instruction::URem:
    if (R.isZero())
      return false;
    Result = L.urem(R);
    return true;

  case Instruction::Shl:
    if (BadShift)
      return false;
    Result = L.shl(R.getZExtValue());
    return true;
  case Instruction::LShr:
    if (BadShift)
      return false;
    Result = L.lshr(R.getZExtValue());
    return true;
  case Instruction::AShr:
    if (BadShift)
      return false;
    Result = L.ashr(R.getZExtValue());
    return true;
  }
  return false;
}

PreservedAnalyses ConstantFoldingPass::run(Function &F,
                                           FunctionAnalysisManager &AM) {
  errs() << "[constant-folding] Function: " << F.getName() << "\n";

  bool Changed = false;
  SmallVector<Instruction *, 16> ToDelete;

  for (BasicBlock &BB : F) {
    for (Instruction &I : BB) {
      if (!isa<BinaryOperator>(I) && !isa<ICmpInst>(I))
        continue;

      auto *L = dyn_cast<ConstantInt>(I.getOperand(0));
      auto *R = dyn_cast<ConstantInt>(I.getOperand(1));
      if (!L || !R)
        continue;

      Constant *Folded = nullptr;
      if (auto *Cmp = dyn_cast<ICmpInst>(&I)) {
        bool Res = ICmpInst::compare(L->getValue(), R->getValue(),
                                     Cmp->getPredicate());
        Folded = ConstantInt::getBool(Cmp->getType(), Res);
      } else {
        APInt Res;
        if (!foldBinary(I.getOpcode(), L->getValue(), R->getValue(), Res))
          continue;
        Folded = ConstantInt::get(I.getType(), Res);
      }

      errs() << "  folded" << I << "  into " << *Folded << "\n";
      I.replaceAllUsesWith(Folded);
      ToDelete.push_back(&I);
      Changed = true;
    }
  }

  for (Instruction *I : ToDelete)
    I->eraseFromParent();

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
