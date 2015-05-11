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
const std::string bugTypeRequestUsage{"invalid request usage"};
const std::string bugTypeUnmatchedWait{"unmatched wait function"};
const std::string bugTypeUnmatchedCall{"unmatched function"};
const std::string bugTypeCollCallInBranch{"collective call inside rank branch"};

/**
 * Get line number for call expression
 *
 * @param call
 *
 * @return line number as string
 */
std::string MPIBugReporter::lineNumberForCallExpr(const CallExpr *call) const {
    std::string lineNo =
        call->getCallee()->getSourceRange().getBegin().printToString(
            bugReporter_.getSourceManager());

    // split written string into parts
    std::vector<std::string> strs = util::split(lineNo, ':');
    return strs.at(strs.size() - 2);
}

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
        bugGroupMPIError, "Buffer type and specified MPI type do not match. ",
        location, {range, callExpr->getArg(idxPair.first)->getSourceRange(),
                   callExpr->getArg(idxPair.second)->getSourceRange()});
}

/**
 * Reports if a collective call is used inside a rank branch.
 * @param callExpr collective call
 */
void MPIBugReporter::reportCollCallInBranch(
    const CallExpr *const callExpr) const {
    auto adc = analysisManager_.getAnalysisDeclContext(currentFunctionDecl_);
    PathDiagnosticLocation location = PathDiagnosticLocation::createBegin(
        callExpr, bugReporter_.getSourceManager(), adc);

    SourceRange range = callExpr->getCallee()->getSourceRange();

    bugReporter_.EmitBasicReport(
        adc->getDecl(), &checkerBase_, bugTypeCollCallInBranch,
        bugGroupMPIError,
        "Collective calls must be executed by all processes."
        " Move this call out of the rank branch. ",
        location, range);
}

void MPIBugReporter::reportUnmatchedCall(const CallExpr *const callExpr,
                                         std::string missingType) const {
    auto adc = analysisManager_.getAnalysisDeclContext(currentFunctionDecl_);
    PathDiagnosticLocation location = PathDiagnosticLocation::createBegin(
        callExpr, bugReporter_.getSourceManager(), adc);

    SourceRange range = callExpr->getCallee()->getSourceRange();

    bugReporter_.EmitBasicReport(
        adc->getDecl(), &checkerBase_, bugTypeUnmatchedCall, bugGroupMPIError,
        "No matching " + missingType + " function found. ", location, range);
}

/**
 * Report non-integer value usage at indices where not allowed.
 *
 * @param callExpr
 * @param idx
 * @param type
 */
void MPIBugReporter::reportInvalidArgumentType(
    const CallExpr *const callExpr, const size_t idx,
    const SourceRange invalidSourceRange,
    const std::string &typeAsString) const {
    auto d = analysisManager_.getAnalysisDeclContext(currentFunctionDecl_);
    PathDiagnosticLocation location = PathDiagnosticLocation::createBegin(
        callExpr, bugReporter_.getSourceManager(), d);

    std::string indexAsString{std::to_string(idx)};
    SourceRange callExprRange = callExpr->getCallee()->getSourceRange();

    bugReporter_.EmitBasicReport(
        d->getDecl(), &checkerBase_, bugTypeInvalidArgumentType,
        bugGroupMPIError,
        typeAsString + " used at index " + indexAsString + " is not valid. ",
        location, {callExprRange, invalidSourceRange});
}

void MPIBugReporter::reportRedundantCall(
    const CallExpr *const matchedCall, const CallExpr *const duplicateCall,
    const llvm::SmallVectorImpl<size_t> &indices) const {
    auto analysisDeclCtx =
        analysisManager_.getAnalysisDeclContext(currentFunctionDecl_);

    PathDiagnosticLocation location = PathDiagnosticLocation::createBegin(
        duplicateCall, bugReporter_.getSourceManager(), analysisDeclCtx);

    std::string lineNo = lineNumberForCallExpr(matchedCall);

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
        "Identical communication arguments used in " +
            matchedCall->getDirectCallee()->getNameAsString() + " in line " +
            lineNo + ".\nConsider to summarize these calls. ",
        location, sourceRanges);
}

void MPIBugReporter::reportDoubleRequestUse(
    const CallExpr *const newCall, const VarDecl *const requestVar,
    const CallExpr *const prevCall) const {
    auto analysisDeclCtx =
        analysisManager_.getAnalysisDeclContext(currentFunctionDecl_);

    PathDiagnosticLocation location = PathDiagnosticLocation::createBegin(
        newCall, bugReporter_.getSourceManager(), analysisDeclCtx);

    std::string lineNo = lineNumberForCallExpr(prevCall);

    bugReporter_.EmitBasicReport(
        analysisDeclCtx->getDecl(), &checkerBase_, bugTypeRequestUsage,
        bugGroupMPIError,
        "Same request variable used in " +
            prevCall->getDirectCallee()->getNameAsString() + " in line " +
            lineNo +
            ".\nUse different request variables or\nmatch the "
            "preceeding call with a wait function before. ",
        location, {newCall->getSourceRange(), requestVar->getSourceRange(),
                   prevCall->getSourceRange()});
}

void MPIBugReporter::reportUnmatchedWait(const CallExpr *const waitCall,
                                         const VarDecl *const varDecl) const {
    auto analysisDeclCtx =
        analysisManager_.getAnalysisDeclContext(currentFunctionDecl_);

    PathDiagnosticLocation location = PathDiagnosticLocation::createBegin(
        waitCall, bugReporter_.getSourceManager(), analysisDeclCtx);

    bugReporter_.EmitBasicReport(
        analysisDeclCtx->getDecl(), &checkerBase_, bugTypeUnmatchedWait,
        bugGroupMPIError,
        "No immediate call is matching request " + varDecl->getNameAsString() +
            ". This will result in an endless wait. ",
        location, {waitCall->getSourceRange(), varDecl->getSourceRange()});
}

}  // end of namespace: mpi
