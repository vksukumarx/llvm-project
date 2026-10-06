/*
 * Test for PASS 3: copy-propagation
 *
 *   opt -passes=copy-propagation -S copy_prop.ll
 *
 * Expected: the loads of y and z are changed into loads of x, so
 * "result = z * 2" really computes "x * 2". The stores into y and z are then
 * useless; run simple-dce afterwards to remove them:
 *   opt -passes="copy-propagation,simple-dce" -S copy_prop.ll
 */
int process_value(int input) {
    int x;
    int y;
    int z;
    int result;

    x = input;          /* copy: x = input                                */
    y = x;              /* copy: y = x                                    */
    z = y;              /* y is x  -> recorded as z = x                   */

    result = z * 2;     /* z is x  -> load of z becomes load of x         */
    return result;
}

/* A copy stops being valid when the original variable changes. */
int copy_killed(int a) {
    int b = a;          /* b = a                                          */
    a = a + 1;          /* a changes -> b is no longer a copy of a        */
    return b;           /* load of b must stay a load of b                */
}
