### Setup
If you used one of the provided setup scripts `unit_tests` was symlinked to
`llvm36/repo/tools/clang/unittests/MPI-Checker`.  Else please do this manually
and add `add_subdirectory(MPI-Checker)` to
`llvm36/repo/tools/clang/unittests/CMakeLists-txt`.

### Run
Run the executable at
`llvm36/build/(debug|release)/tools/clang/unittests/MPI-Checker`
