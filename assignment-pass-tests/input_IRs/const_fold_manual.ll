; Hand-written test for PASS 2: constant-folding
;
;   opt -passes=constant-folding -S const_fold_manual.ll
;
; Expected: every arithmetic instruction and the icmp disappear.
;   %a = 5, %b = 20, %c = 14, %d = (14 > 10) = true
;   => "br i1 true, ..." and "ret i32 14"
; Then run simple-dce as well to remove the block that can never be reached:
;   opt -passes="constant-folding,simple-dce" -S const_fold_manual.ll

define i32 @fold_demo() {
entry:
  %a = add i32 2, 3
  %b = mul i32 %a, 4
  %c = sub i32 %b, 6
  %d = icmp sgt i32 %c, 10
  br i1 %d, label %yes, label %no

yes:
  ret i32 %c

no:
  ret i32 0
}

; Division by zero must NOT be folded (it is undefined behaviour).
define i32 @no_fold_div0() {
entry:
  %x = sdiv i32 7, 0
  ret i32 %x
}
