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
            typeAsString = "Literal";
            break;

        case InvalidArgType::kVariable:
            typeAsString = "Variable";
            break;

        case InvalidArgType::kReturnType:
            typeAsString = "Return value from function";
            break;
    }

    bugReporter_.EmitBasicReport(
        d->getDecl(), &checkerBase_, bugTypeInvalidArgumentType,
        bugGroupMPIError,
        typeAsString + " used at index " + indexAsString + " is not valid. ",
        location, {callExprRange, invalidSourceRange});
}

void MPIBugReporter::reportRedundantCall(
    const CallExpr *matchedCall, const CallExpr *duplicateCall,
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

std::string MPIBugReporter::lineNumberForCallExpr(const CallExpr *call) const {
    std::string lineNo =
        call->getCallee()->getSourceRange().getBegin().printToString(
            bugReporter_.getSourceManager());

    // split written string into parts
    std::vector<std::string> strs = util::split(lineNo, ':');
    return strs.at(strs.size() - 2);
}

void MPIBugReporter::reportDoubleRequestUse(const CallExpr *newCall,
                                            const VarDecl *requestVar,
                                            const CallExpr *prevCall) const {
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

void MPIBugReporter::reportUnmatchedWait(const CallExpr *waitCall,
        const VarDecl *varDecl) const {
    auto analysisDeclCtx =
        analysisManager_.getAnalysisDeclContext(currentFunctionDecl_);

    PathDiagnosticLocation location = PathDiagnosticLocation::createBegin(
        waitCall, bugReporter_.getSourceManager(), analysisDeclCtx);

    bugReporter_.EmitBasicReport(analysisDeclCtx->getDecl(), &checkerBase_,
                                 bugTypeUnmatchedWait, bugGroupMPIError,
                                 "No immediate call is matching request "
                                 + varDecl->getNameAsString() +
                                 ". This will result in an endless wait. ",
                                 location, {waitCall->getSourceRange(),
                                 varDecl->getSourceRange()});
}

}  // end of namespace: mpi
