/*
 * Test for PASS 6: simple-instcombine
 *
 *   opt -passes=simple-instcombine -S instcombine.ll
 */

int times8(int x)  { return x * 8; }    /* mul x, 8  -> shl x, 3        */
int times16(int x) { return 16 * x; }   /* constant on the left works too
                                           -> shl x, 4                   */
int times1(int x)  { return x * 1; }    /* mul x, 1  -> x                */
int times0(int x)  { return x * 0; }    /* mul x, 0  -> 0                */
int times6(int x)  { return x * 6; }    /* 6 is not a power of 2: kept   */

unsigned udiv4(unsigned u) { return u / 4; }  /* udiv u, 4 -> lshr u, 2   */
int      sdiv4(int x)      { return x / 4; }  /* signed: NOT changed
                                                 (-7/2 = -3 but -7>>1 = -4) */
