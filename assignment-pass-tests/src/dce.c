/*
 * Test for PASS 5: simple-dce
 *
 * 1) Unused computation - on its own:
 *      opt -passes=simple-dce -S dce.ll
 *    Expected: the variable 'unused', its store and the "a * b" are removed.
 *
 * 2) Unreachable code - together with the constant passes, which turn
 *    "if (flag)" into "br i1 false, ...":
 *      opt -passes="constant-propagation,constant-folding,simple-dce" -S dce.ll
 *    Expected: the block with "result * 100" is removed as well.
 */
int dce(int a, int b) {
    int unused = a * b;    /* never read -> unused computation            */
    int flag = 0;
    int result = a + b;

    if (flag) {            /* flag is always 0 -> this block never runs   */
        result = result * 100;
    }
    return result;
}
