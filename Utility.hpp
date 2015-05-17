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
