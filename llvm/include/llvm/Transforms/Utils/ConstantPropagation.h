#ifndef LLVM_TRANSFORMS_UTILS_CONSTANTPROPAGATION_H
#define LLVM_TRANSFORMS_UTILS_CONSTANTPROPAGATION_H

#include "llvm/IR/PassManager.h"

namespace llvm {

class ConstantPropagationPass
    : public OptionalPassInfoMixin<ConstantPropagationPass> {
public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
};

} // namespace llvm

#endif // LLVM_TRANSFORMS_UTILS_CONSTANTPROPAGATION_H
