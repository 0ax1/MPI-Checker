/*
 The MIT License (MIT)

 Copyright (c) 2015 Alexander Droste

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
*/

#ifndef UTILITY_HPP_SVQZWTL8
#define UTILITY_HPP_SVQZWTL8

#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"

namespace util {

/**
 * Returns part of the code specified by range unmodified as string ref.
 *
 * @param source range
 * @param analysis manager
 *
 * @return code part as string ref
 */
clang::StringRef sourceRangeAsStringRef(const clang::SourceRange &,
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

/**
 * Retrieve identifier info for a call expression.
 * Returns nullptr if there's no direct callee.
 *
 * @param callExpr
 *
 * @return
 */
const clang::IdentifierInfo *getIdentInfo(const clang::CallExpr *);

}  // end of namespace: util

#endif  // end of include guard: UTILITY_HPP_SVQZWTL8
