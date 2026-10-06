#ifndef LLVM_TRANSFORMS_UTILS_SIMPLEDCE_H
#define LLVM_TRANSFORMS_UTILS_SIMPLEDCE_H

#include "llvm/IR/PassManager.h"

namespace llvm {

class SimpleDCEPass : public OptionalPassInfoMixin<SimpleDCEPass> {
public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
};

} // namespace llvm

#endif // LLVM_TRANSFORMS_UTILS_SIMPLEDCE_H
