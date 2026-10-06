#ifndef LLVM_TRANSFORMS_UTILS_CONSTANTFOLDINGPASS_H
#define LLVM_TRANSFORMS_UTILS_CONSTANTFOLDINGPASS_H

#include "llvm/IR/PassManager.h"

namespace llvm {

class ConstantFoldingPass : public OptionalPassInfoMixin<ConstantFoldingPass> {
public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
};

} // namespace llvm

#endif // LLVM_TRANSFORMS_UTILS_CONSTANTFOLDINGPASS_H
