/*
 * Test for PASS 4: local-cse
 *
 *   opt -passes=local-cse -S cse.ll
 *
 * Expected: only ONE "mul" is left. The second loads of b and c are
 * replaced by the first ones, then the second "b * c" is replaced by the
 * first "b * c":
 *     tmp = b * c;  y = tmp;  z = tmp + d;
 */
int cse(int b, int c, int d) {
    int y = b * c;
    int z = b * c + d;     /* b * c is computed again -> reused            */
    int w = c * b;         /* same as b * c (multiplication is commutative) */
    return y + z + w;
}

/* The expression is NOT reused when a variable changes in between. */
int cse_killed(int b, int c) {
    int y = b * c;
    b = b + 1;             /* b changes: the next b * c is a new value      */
    int z = b * c;
    return y + z;
}
