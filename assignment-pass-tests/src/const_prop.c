/*
 * Test for PASS 1: constant-propagation
 *
 *   opt -passes=constant-propagation -S const_prop.ll
 */

/* Everything is in ONE basic block, so every read of a variable that holds
 * a constant is replaced by that constant. */
int const_prop(void) {
    int a = 10;       /* store i32 10, ptr %a                               */
    int b = a;        /* load of a  -> replaced by 10  => store i32 10, %b  */
    int c = b + a;    /* loads of b and a -> 10        => add i32 10, 10    */
    return c;         /* c holds "add 10, 10" - not a constant yet; running
                         constant-folding afterwards turns it into 20       */
}

/* The pass is LOCAL: the 'if' body is a different basic block, so the load
 * of x inside it is NOT replaced, even though x is 5 there. The load of x
 * in the entry block (before the branch) is replaced. */
int const_prop_local(int n) {
    int x = 5;
    int y = x * 2;    /* same block as "x = 5"  -> load of x replaced by 5  */
    if (n > 0) {
        y = x + 1;    /* new block -> load of x is kept                     */
    }
    return y;
}
