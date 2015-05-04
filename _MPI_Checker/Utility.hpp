#ifndef UTILITY_HPP_SVQZWTL8
#define UTILITY_HPP_SVQZWTL8

#include "clang/StaticAnalyzer/Core/Checker.h"
#include "clang/Lex/Lexer.h"
#include "clang/Analysis/AnalysisContext.h"
#include "clang/Basic/LangOptions.h"

#include "../ClangSACheckers.h"
#include "clang/StaticAnalyzer/Core/Checker.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"
#include "clang/AST/RecursiveASTVisitor.h"

#include <sstream>
#include <vector>
#include "Container.hpp"

namespace util {

/**
 * Checks if two containers are permutations of each other.
 *
 * @tparam T1 type of container 1
 * @tparam T2 type of container 2
 * @param first container
 * @param second container
 *
 * @return isPermutation
 */
template <typename T1, typename T2>
bool isPermutation(const T1 &first, const T2 &second) {
    // size must match
    if (first.size() != second.size()) return false;

    // copy because matches get erased
    auto copy = first;
    for (auto &componentFromSecond : second) {
        if (!cont::isContained(copy, componentFromSecond)) {
            return false;
        } else {
            // to omit double matching
            cont::erase(copy, componentFromSecond);
        }
    }
    return true;
}

/**
 * Check if variable decl has a typedef type.
 *
 * @param vardecl to check
 *
 * @return
 */
bool isTypedef(const clang::VarDecl *);

/**
 * Returns part of the code specified by range unmodified as string ref.
 *
 * @param source range
 * @param analysis manager
 *
 * @return code part as string ref
 */
clang::StringRef sourceRangeAsStringRef(clang::SourceRange,
                                   clang::ento::AnalysisManager &);

/**
 * Split string by delimiter.
 *
 * @param string to split
 * @param delimiter
 *
 * @return string array split by delimiter
 */
std::vector<std::string> split(const std::string &, char);

}  // end of namespace: util

#endif  // end of include guard: UTILITY_HPP_SVQZWTL8
