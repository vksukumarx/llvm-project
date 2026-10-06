#ifndef LLVM_TRANSFORMS_UTILS_SIMPLEINSTCOMBINE_H
#define LLVM_TRANSFORMS_UTILS_SIMPLEINSTCOMBINE_H

#include "llvm/IR/PassManager.h"

namespace llvm {

class SimpleInstCombinePass
    : public OptionalPassInfoMixin<SimpleInstCombinePass> {
public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
};

} // namespace llvm

#endif // LLVM_TRANSFORMS_UTILS_SIMPLEINSTCOMBINE_H
