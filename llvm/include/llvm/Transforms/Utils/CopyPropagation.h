#ifndef LLVM_TRANSFORMS_UTILS_COPYPROPAGATION_H
#define LLVM_TRANSFORMS_UTILS_COPYPROPAGATION_H

#include "llvm/IR/PassManager.h"

namespace llvm {

class CopyPropagationPass : public OptionalPassInfoMixin<CopyPropagationPass> {
public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
};

} // namespace llvm

#endif // LLVM_TRANSFORMS_UTILS_COPYPROPAGATION_H
