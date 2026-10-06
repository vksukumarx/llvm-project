@echo off
REM ==========================================================================
REM  run_tests.bat - runs each assignment pass on its own sample IR.
REM
REM  Usage:
REM      run_tests.bat          run the passes on input_IRs\*.ll
REM      run_tests.bat regen    first re-create input_IRs\*.ll from src\*.c,
REM                             then run the passes
REM
REM  Input  : input_IRs\NAME.ll       (-O0 IR, committed to the repo)
REM  Output : output\NAME.opt.ll      (IR after the pass)
REM  The messages printed by each pass are shown on the screen.
REM
REM  By default the tools are taken from the build folder of this repo
REM  (llvm-project\build\bin). To use another folder, set LLVM_BIN first:
REM      set LLVM_BIN=D:\my-build\bin
REM ==========================================================================
setlocal

REM ---- 0. setup ------------------------------------------------------------
REM %~dp0 is the folder of this script (ends with '\').
cd /d "%~dp0"
if "%LLVM_BIN%"=="" set "LLVM_BIN=%~dp0..\build\bin"

if not exist "%LLVM_BIN%\opt.exe" (
    echo ERROR: opt.exe not found in "%LLVM_BIN%".
    echo Build it first, or set LLVM_BIN to the folder that has opt.exe.
    exit /b 1
)

set OPT="%LLVM_BIN%\opt.exe" -S
if not exist output mkdir output

REM ---- 1. (optional) re-create the input IR from the C samples -------------
REM  -Xclang -disable-O0-optnone : clang -O0 normally marks every function
REM  'optnone', and the pass manager then skips our (optional) passes.
REM  -fdebug-compilation-dir=. and -fno-ident keep machine-specific paths and
REM  the compiler version out of the generated files.
if /i "%1"=="regen" (
    echo ===== re-generating input_IRs from src =====
    pushd src
    for %%f in (const_prop const_fold copy_prop cse dce instcombine) do (
        "%LLVM_BIN%\clang.exe" -O0 -Xclang -disable-O0-optnone -fno-ident -g0 -fdebug-compilation-dir=. -S -emit-llvm %%f.c -o ..\input_IRs\%%f.ll || goto :error
    )
    popd
)

REM ---- 2. run the passes ----------------------------------------------------
REM  Each 'opt' writes the new IR to output\NAME.opt.ll. The pass messages
REM  are printed on the screen.

echo.
echo ===== 1. constant-propagation =====
%OPT% -passes=constant-propagation input_IRs\const_prop.ll -o output\const_prop.opt.ll || goto :error

echo.
echo ===== 2. constant-folding (together with constant-propagation) =====
REM  Folding needs constants as operands, so propagation runs first.
REM  Running the pair twice lets a folded result be propagated and folded again.
%OPT% -passes="constant-propagation,constant-folding,constant-propagation,constant-folding" input_IRs\const_fold.ll -o output\const_fold.opt.ll || goto :error
echo ----- constant-folding on the hand-written IR -----
%OPT% -passes=constant-folding input_IRs\const_fold_manual.ll -o output\const_fold_manual.opt.ll || goto :error

echo.
echo ===== 3. copy-propagation =====
%OPT% -passes=copy-propagation input_IRs\copy_prop.ll -o output\copy_prop.opt.ll || goto :error

echo.
echo ===== 4. local-cse =====
%OPT% -passes=local-cse input_IRs\cse.ll -o output\cse.opt.ll || goto :error

echo.
echo ===== 5. simple-dce (with the constant passes, for the unreachable 'if') =====
%OPT% -passes="constant-propagation,constant-folding,simple-dce" input_IRs\dce.ll -o output\dce.opt.ll || goto :error

echo.
echo ===== 6. simple-instcombine =====
%OPT% -passes=simple-instcombine input_IRs\instcombine.ll -o output\instcombine.opt.ll || goto :error

echo.
echo All passes ran. Compare input_IRs\NAME.ll (before) with output\NAME.opt.ll (after).
exit /b 0

:error
echo FAILED - see the message above.
exit /b 1
