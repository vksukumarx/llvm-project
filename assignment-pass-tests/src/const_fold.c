/*
 * Test for PASS 2: constant-folding
 *
 * clang already folds pure literals like "10 + 20" by itself, so here the
 * constants only appear after constant-propagation has replaced 'a' by 10.
 * Run the two passes together:
 *
 *   opt -passes="constant-propagation,constant-folding" -S const_fold.ll
 *
 * For folding on its own, use the hand-written const_fold_manual.ll.
 */
int const_fold(void) {
    int a = 10;
    int b = a * 3 + 4;   /* after propagation: mul 10, 3 ; add %m, 4
                            after folding:     store i32 34, ptr %b        */
    int c = b - 4;       /* running the pair again gives: store i32 30     */
    return c;
}
