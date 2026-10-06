#ifndef LLVM_TRANSFORMS_UTILS_LOCALCSE_H
#define LLVM_TRANSFORMS_UTILS_LOCALCSE_H

#include "llvm/IR/PassManager.h"

namespace llvm {

class LocalCSEPass : public OptionalPassInfoMixin<LocalCSEPass> {
public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
};

} // namespace llvm

#endif // LLVM_TRANSFORMS_UTILS_LOCALCSE_H
