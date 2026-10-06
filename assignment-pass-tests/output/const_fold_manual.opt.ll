; ModuleID = 'input_IRs\const_fold_manual.ll'
source_filename = "input_IRs\\const_fold_manual.ll"

define i32 @fold_demo() {
entry:
  br i1 true, label %yes, label %no

yes:                                              ; preds = %entry
  ret i32 14

no:                                               ; preds = %entry
  ret i32 0
}

define i32 @no_fold_div0() {
entry:
  %x = sdiv i32 7, 0
  ret i32 %x
}
