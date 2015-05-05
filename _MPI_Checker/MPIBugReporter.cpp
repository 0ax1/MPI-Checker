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
void MPIBugReporter::reportTypeMismatch(CallExpr *callExpr) const {
    auto adc = analysisManager_.getAnalysisDeclContext(currentFunctionDecl_);
    PathDiagnosticLocation location = PathDiagnosticLocation::createBegin(
        callExpr, bugReporter_.getSourceManager(), adc);

    SourceRange range = callExpr->getCallee()->getSourceRange();

    bugReporter_.EmitBasicReport(
        adc->getDecl(), &checkerBase_, bugTypeArgumentTypeMismatch,
        bugGroupMPIError, "buffer type and specified mpi type do not match",
        location, range);
}

/**
 * Report non-integer value usage at indices where not allowed.
 *
 * @param callExpr
 * @param idx
 * @param type
 */
void MPIBugReporter::reportInvalidArgumentType(CallExpr *callExpr, size_t idx,
                                               InvalidArgType type) const {
    auto d = analysisManager_.getAnalysisDeclContext(currentFunctionDecl_);
    PathDiagnosticLocation location = PathDiagnosticLocation::createBegin(
        callExpr, bugReporter_.getSourceManager(), d);

    std::string indexAsString{std::to_string(idx)};
    SourceRange range = callExpr->getCallee()->getSourceRange();

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
        location, range);
}

void MPIBugReporter::reportDuplicate(const CallExpr *matchedCall,
                                     const CallExpr *duplicateCall) const {
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

    SourceRange range = duplicateCall->getCallee()->getSourceRange();

    bugReporter_.EmitBasicReport(
        analysisDeclCtx->getDecl(), &checkerBase_, bugTypeEfficiency,
        bugGroupMPIWarning,
        "identical communication "
        "arguments (count, mpi-datatype, rank, tag) used in " +
            matchedCall->getDirectCallee()->getNameAsString() + " in line: " +
            lineNo + " \n\nconsider to summarize these calls",
        location, range);
}

}  // end of namespace: mpi
