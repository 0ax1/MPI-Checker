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

#include "clang/Lex/Lexer.h"
#include "clang/StaticAnalyzer/Core/CheckerManager.h"
#include "Utility.hpp"
#include <sstream>
#include <vector>

namespace util {

/**
 * Split string by delimiter helper function.
 */
static std::vector<std::string> &split(const std::string &s, char delim,
                                       std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
}

clang::StringRef sourceRangeAsStringRef(
    const clang::SourceRange &sourceRange,
    clang::ento::AnalysisManager &analysisManager) {
    auto charSourceRange = clang::CharSourceRange::getTokenRange(sourceRange);
    return clang::Lexer::getSourceText(charSourceRange,
                                       analysisManager.getSourceManager(),
                                       clang::LangOptions());
}


clang::SourceRange sourceRange(const clang::ento::MemRegion *memRegion) {
    const clang::ento::VarRegion *varRegion =
        clang::dyn_cast<clang::ento::VarRegion>(memRegion->getBaseRegion());

    return varRegion->getDecl()->getSourceRange();
}

std::string variableName(const clang::ento::MemRegion *memRegion) {
    const clang::ento::VarRegion *varRegion =
        clang::dyn_cast<clang::ento::VarRegion>(memRegion->getBaseRegion());
    std::string varName = varRegion->getDecl()->getNameAsString();
    bool isElementInArray = varRegion->getValueType()->isArrayType();

    if (isElementInArray) {
        auto elementRegion = memRegion->getAs<clang::ento::ElementRegion>();
        llvm::APSInt indexInArray;
        indexInArray = elementRegion->getIndex()
                           .getAs<clang::ento::nonloc::ConcreteInt>()
                           ->getValue();

        llvm::SmallVector<char, 2> intValAsString;
        indexInArray.toString(intValAsString);
        std::string idx;
        for (char c : intValAsString) {
            idx.push_back(c);
        }
        return varName + "[" + idx + "]";
    } else {
        return varName;
    }
}

const clang::IdentifierInfo *getIdentInfo(const clang::CallExpr *callExpr) {
    if (callExpr->getDirectCallee()) {
        return callExpr->getDirectCallee()->getIdentifier();
    } else {
        return nullptr;
    }
}

}  // end of namespace: util
