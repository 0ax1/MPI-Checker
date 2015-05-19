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

}  // end of namespace: util
