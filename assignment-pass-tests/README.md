# Assignment: Six Local Optimization Passes in LLVM

This folder holds the test programs and the test script for six LLVM
function passes I wrote for the course assignment. Every pass is
**local**: it looks at one basic block at a time. Each pass still runs on
**every** basic block of the function, but what it learned in one block is
forgotten when the next block starts.

| # | Pass (`-passes=` name)   | What it does                                                               | Source file                    |
|---|--------------------------|----------------------------------------------------------------------------|--------------------------------|
| 1 | `constant-propagation`   | Replaces a read of a variable that holds a known constant with that constant | `ConstantPropagation.cpp`      |
| 2 | `constant-folding`       | Computes instructions whose operands are all constants (`2*3+4` → `10`)    | `ConstantFoldingPass.cpp`      |
| 3 | `copy-propagation`       | After `y = x`, a read of `y` becomes a read of `x`                         | `CopyPropagation.cpp`          |
| 4 | `local-cse`              | Reuses an expression that was already computed (`b*c` computed once)       | `LocalCSE.cpp`                 |
| 5 | `simple-dce`             | Removes unreachable blocks, write-only variables and unused instructions   | `SimpleDCE.cpp`                |
| 6 | `simple-instcombine`     | Turns multiply/unsigned-divide by a power of 2 into a shift (`x*8` → `x<<3`) | `SimpleInstCombine.cpp`        |

---

## 1. Where the code is

| What                     | Path (from the root of `llvm-project`)                          |
|--------------------------|-----------------------------------------------------------------|
| Pass headers             | `llvm/include/llvm/Transforms/Utils/<Name>.h`                   |
| Pass sources             | `llvm/lib/Transforms/Utils/<Name>.cpp`                          |
| Added to the build       | `llvm/lib/Transforms/Utils/CMakeLists.txt`                      |
| Registered as `-passes=` | `llvm/lib/Passes/PassRegistry.def` (`FUNCTION_PASS(...)` lines) |
| Header includes          | `llvm/lib/Passes/PassBuilder.cpp`                               |
| Tests (this folder)      | `assignment-pass-tests/`                                        |

Each `.cpp` file starts with a short comment describing what the pass does.

## 2. This folder

```
assignment-pass-tests/
  README.md        this file
  run_tests.bat    runs every pass on its input IR
  src/             C sample programs, one per pass
  input_IRs/       -O0 LLVM IR made from src/*.c  (input to opt)
  output/          NAME.opt.ll - IR after the pass (written by the script)
```

`input_IRs/const_fold_manual.ll` is written by hand and has no C source.
clang already folds plain literals like `2 + 3` while it generates IR, so
the hand-written file gives the folding pass constants to work on by itself.

## 3. Build

The script needs `opt.exe` (and `clang.exe` for the `regen` option). Build
them from an **x64 Native Tools Command Prompt for Visual Studio**:

```bat
cd llvm-project\build
cmake --build . --target opt
cmake --build . --target clang      &REM only needed for "regen"
```

## 4. Run

```bat
cd llvm-project\assignment-pass-tests
run_tests.bat
```

| Command                         | Effect                                                                   |
|---------------------------------|--------------------------------------------------------------------------|
| `run_tests.bat`                 | Runs the passes on `input_IRs\*.ll` and writes the results to `output\`  |
| `run_tests.bat regen`           | First rebuilds `input_IRs\*.ll` from `src\*.c` with clang, then runs the passes |
| `set LLVM_BIN=D:\x\bin` before  | Uses `opt`/`clang` from another folder. The default is `..\build\bin`    |

You can also run one pass by hand:

```bat
..\build\bin\opt.exe -S -passes=local-cse input_IRs\cse.ll -o output\cse.opt.ll
```

## 5. What the script does

1. **Setup.** It changes to the script's folder and finds `opt.exe` in
   `LLVM_BIN`, which defaults to `..\build\bin`. It stops with an error if
   `opt.exe` is missing, and it creates `output\`.
2. **Regenerate the inputs (only with `regen`).** It compiles every `src\*.c`
   file to IR with these flags:
   ```
   clang -O0 -Xclang -disable-O0-optnone -fno-ident -g0 -fdebug-compilation-dir=. -S -emit-llvm
   ```
   - At `-O0`, clang normally adds the `optnone` attribute to every
     function. The new pass manager then **skips** optional passes like
     ours. `-disable-O0-optnone` keeps the simple `-O0` code shape
     (`alloca`/`load`/`store` for every variable) but removes `optnone`, so
     the passes really run.
   - `-fno-ident` and `-fdebug-compilation-dir=.` keep the compiler version
     and local folder paths out of the committed files.
3. **Run the passes.** For each test it calls
   `opt -S -passes=<pipeline> input_IRs\NAME.ll -o output\NAME.opt.ll`. The
   messages printed by the pass appear on the screen.
4. **Finish.** It prints `All passes ran.` If `opt` fails, it prints
   `FAILED` and returns exit code 1.

## 6. The tests and what to look for

To see what a pass changed, compare `input_IRs\NAME.ll` (before) with
`output\NAME.opt.ll` (after).

| Input IR               | Pipeline                                                                      | What to check in the output                                           |
|------------------------|-------------------------------------------------------------------------------|-----------------------------------------------------------------------|
| `const_prop.ll`        | `constant-propagation`                                                        | Loads of `a`/`b` become `10`. In `const_prop_local`, the load of `x` inside the `if` block is **kept** (other block) |
| `const_fold.ll`        | `constant-propagation,constant-folding,constant-propagation,constant-folding` | `b` becomes `store i32 34`, `c` becomes `store i32 30`                 |
| `const_fold_manual.ll` | `constant-folding`                                                            | `fold_demo` ends in `br i1 true` and `ret i32 14`. `sdiv 7, 0` is **not** folded |
| `copy_prop.ll`         | `copy-propagation`                                                            | Loads of `y` and `z` read `x`'s slot. In `copy_killed`, the load of `b` is kept |
| `cse.ll`               | `local-cse`                                                                   | `cse` has only **one** `mul`. In `cse_killed`, the second `mul` is kept |
| `dce.ll`               | `constant-propagation,constant-folding,simple-dce`                            | The `if (flag)` block, the `unused` variable and its `mul` are gone    |
| `instcombine.ll`       | `simple-instcombine`                                                          | `mul 8` → `shl 3`, `mul 16` → `shl 4`, `udiv 4` → `lshr 2`. `x*6` and signed `/4` are unchanged |

Folding and DCE are chained with constant propagation. clang does not
emit constant arithmetic or `if (0)` branches at `-O0` by itself; the
constants only appear after propagation has replaced the variables.

### Before / after examples

`-O0` values have no names, so they are shown as `%N`. The variable names
in the comments come from the C source.

**1. constant-propagation** (`const_prop`)
```llvm
; before                                   ; after
store i32 10, ptr %1          ; a = 10     store i32 10, ptr %1
%4 = load i32, ptr %1                      store i32 10, ptr %2        ; b = 10
store i32 %4, ptr %2          ; b = a      %4 = add i32 10, 10
```

**2. constant-folding** (`fold_demo`)
```llvm
; before                                   ; after
%a = add i32 2, 3                          br i1 true, label %yes, label %no
%b = mul i32 %a, 4                         ...
%c = sub i32 %b, 6                         ret i32 14
%d = icmp sgt i32 %c, 10
br i1 %d, label %yes, label %no
```

**3. copy-propagation** (`process_value`: `y = x; z = y; result = z * 2`)
```llvm
; before                                   ; after
%8 = load i32, ptr %3         ; read x     %8 = load i32, ptr %2       ; reads input
...                                        ...
%10 = load i32, ptr %5        ; read z     %10 = load i32, ptr %2      ; reads input
%11 = mul i32 %10, 2                       %11 = mul i32 %10, 2
```

**4. local-cse** (`cse`: `y = b*c; z = b*c + d; w = c*b`)
```llvm
; before                                   ; after
%12 = mul i32 %10, %11        ; b*c        %12 = mul i32 %10, %11
...                                        store i32 %12, ptr %7       ; y
%15 = mul i32 %13, %14        ; b*c again  %14 = add i32 %12, %13      ; z = tmp + d
...                                        store i32 %12, ptr %9       ; w
%20 = mul i32 %18, %19        ; c*b
```

**5. simple-dce** (`dce`)
```llvm
; before                                   ; after
%10 = mul i32 %8, %9          ; unused     (removed, with the 'unused' variable)
store i32 %10, ptr %5
store i32 0, ptr %6           ; flag = 0   (removed, with the 'flag' variable)
...
br i1 %15, label %16, label %19            br label %9
16:   ; result = result * 100              (block removed - unreachable)
```

**6. simple-instcombine** (`times8`)
```llvm
; before                                   ; after
%4 = mul i32 %3, 8                         %4 = shl i32 %3, 3
```

### Reading the log

Each pass prints the function name, then one line per change:

```
[simple-instcombine] Function: times8
  replaced  %5 = mul i32 %3, 8  with   %4 = shl i32 %3, 3
```

Unnamed values are numbered again every time LLVM prints something, so the
same `%N` can show up in two lines for two different values. For example,
the DCE log shows `removing write-only variable %5` twice. These are the
variables `unused` and `flag`.
