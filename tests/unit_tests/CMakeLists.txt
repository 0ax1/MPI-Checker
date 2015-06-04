set(LLVM_LINK_COMPONENTS Support)

include_directories(../../lib/StaticAnalyzer/Checkers/MPI-Checker/src)
include_directories(../../src)

add_clang_unittest(MPI-Checker
    UtilityTest.cpp
    ArrayVisitorTest.cpp
    ContainerTest.cpp
    )

target_link_libraries(MPI-Checker
  clangAST
  clangBasic
  clangLex
  clangParse
  clangSema
  clangStaticAnalyzerCheckers)
