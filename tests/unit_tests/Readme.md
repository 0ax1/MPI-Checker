### Setup
Set `option(LLVM_BUILD_TESTS)` and `option(LLVM_INCLUDE_TESTS)` to `ON` in
`llvm36/repo/CMakeLists.txt`.

Then run `ninja ClangUnitTests` in `llvm36/build/(debug|release).

### Run
Run the executable build at
`llvm36/build/(debug|release)/tools/clang/unittests/MPI-Checker`
