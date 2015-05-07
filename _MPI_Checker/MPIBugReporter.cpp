#include "MPIBugReporter.hpp"
#include "Utility.hpp"

using namespace clang;
using namespace ento;

namespace mpi {

const std::string bugGroupMPIError{"MPI Error"};
const std::string bugGroupMPIWarning{"MPI Warning"};

const std::string bugTypeEfficiency{"schema efficiency"};
const std::string bugTypeInvalidArgumentType{"invalid argument type"};
const std::string bugTypeArgumentTypeMismatch{"buffer type mismatch"};

// bug reports–––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

/**
 * Reports mismach between buffer type and mpi datatype.
 * @param callExpr
 */
void MPIBugReporter::reportTypeMismatch(
    const CallExpr *callExpr, const std::pair<size_t, size_t> &idxPair) const {
    auto adc = analysisManager_.getAnalysisDeclContext(currentFunctionDecl_);
    PathDiagnosticLocation location = PathDiagnosticLocation::createBegin(
        callExpr, bugReporter_.getSourceManager(), adc);

    SourceRange range = callExpr->getCallee()->getSourceRange();

    bugReporter_.EmitBasicReport(
        adc->getDecl(), &checkerBase_, bugTypeArgumentTypeMismatch,
        bugGroupMPIError, "buffer type and specified mpi type do not match",
        location, {range, callExpr->getArg(idxPair.first)->getSourceRange(),
                   callExpr->getArg(idxPair.second)->getSourceRange()});
}

/**
 * Report non-integer value usage at indices where not allowed.
 *
 * @param callExpr
 * @param idx
 * @param type
 */
void MPIBugReporter::reportInvalidArgumentType(CallExpr *callExpr, size_t idx,
                                               SourceRange invalidSourceRange,
                                               InvalidArgType type) const {
    auto d = analysisManager_.getAnalysisDeclContext(currentFunctionDecl_);
    PathDiagnosticLocation location = PathDiagnosticLocation::createBegin(
        callExpr, bugReporter_.getSourceManager(), d);

    std::string indexAsString{std::to_string(idx)};
    SourceRange callExprRange = callExpr->getCallee()->getSourceRange();

    std::string typeAsString;
    switch (type) {
        case InvalidArgType::kLiteral:
            typeAsString = "literal";
            break;

        case InvalidArgType::kVariable:
            typeAsString = "variable";
            break;

        case InvalidArgType::kReturnType:
            typeAsString = "return value from function";
            break;
    }

    bugReporter_.EmitBasicReport(
        d->getDecl(), &checkerBase_, bugTypeInvalidArgumentType,
        bugGroupMPIError,
        typeAsString + " used at index " + indexAsString + " is not valid",
        location, {callExprRange, invalidSourceRange});
}

void MPIBugReporter::reportDuplicate(
    const CallExpr *matchedCall, const CallExpr *duplicateCall,
    const llvm::SmallVectorImpl<size_t> &indices) const {
    auto analysisDeclCtx =
        analysisManager_.getAnalysisDeclContext(currentFunctionDecl_);

    PathDiagnosticLocation location = PathDiagnosticLocation::createBegin(
        duplicateCall, bugReporter_.getSourceManager(), analysisDeclCtx);

    std::string lineNo =
        matchedCall->getCallee()->getSourceRange().getBegin().printToString(
            bugReporter_.getSourceManager());

    // split written string into parts
    std::vector<std::string> strs = util::split(lineNo, ':');
    lineNo = strs.at(strs.size() - 2);

    // build source ranges vector
    SmallVector<SourceRange, 10> sourceRanges{
        matchedCall->getCallee()->getSourceRange(),
        duplicateCall->getCallee()->getSourceRange()};

    for (size_t idx : indices) {
        sourceRanges.push_back(matchedCall->getArg(idx)->getSourceRange());
        sourceRanges.push_back(duplicateCall->getArg(idx)->getSourceRange());
    }

    bugReporter_.EmitBasicReport(
        analysisDeclCtx->getDecl(), &checkerBase_, bugTypeEfficiency,
        bugGroupMPIWarning,
        "identical communication arguments used in " +
            matchedCall->getDirectCallee()->getNameAsString() + " in line " +
            lineNo + "\nconsider to summarize these calls",
        location, sourceRanges);
}

}  // end of namespace: mpi
