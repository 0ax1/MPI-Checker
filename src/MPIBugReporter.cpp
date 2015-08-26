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

#include "clang/StaticAnalyzer/Core/PathSensitive/CallEvent.h"
#include "MPIBugReporter.hpp"
#include "Utility.hpp"

using namespace clang;
using namespace ento;

namespace mpi {

const std::string MPIError{"MPI Error"};
const std::string MPIWarning{"MPI Warning"};

/**
 * Get line number for call event ref
 * @param call
 * @return line number as string
 */
std::string MPIBugReporter::lineNumber(
    const CallEventRef<> callEventRef) const {
    std::string lineNo =
        callEventRef->getSourceRange().getBegin().printToString(
            bugReporter_.getSourceManager());

    // split written string into parts
    std::vector<std::string> strs = util::split(lineNo, ':');
    return util::split(lineNo, ':').at(strs.size() - 2);
}

// bug reports ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

// ast reports ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

/**
 * Reports unreachable calls.
 * @param call to report
 */
void MPIBugReporter::reportNotReachableCall(
    const CallExpr *const callExpr) const {
    auto adc = analysisManager_.getAnalysisDeclContext(currentFunctionDecl_);
    PathDiagnosticLocation location = PathDiagnosticLocation::createBegin(
        callExpr, bugReporter_.getSourceManager(), adc);

    SourceRange range = callExpr->getCallee()->getSourceRange();
    std::string bugName{"unreachable call"};
    std::string errorText{
        "Call is not reachable. Schema leads to a deadlock. "};

    bugReporter_.EmitBasicReport(adc->getDecl(), &checkerBase_, bugName,
                                 MPIError, errorText, location, range);
}

/**
 * Reports mismatch between buffer type and mpi datatype.
 * @param callExpr
 */
void MPIBugReporter::reportTypeMismatch(
    const CallExpr *callExpr, const std::pair<size_t, size_t> &idxPair) const {
    auto adc = analysisManager_.getAnalysisDeclContext(currentFunctionDecl_);
    PathDiagnosticLocation location = PathDiagnosticLocation::createBegin(
        callExpr, bugReporter_.getSourceManager(), adc);

    SourceRange callRange = callExpr->getCallee()->getSourceRange();
    std::string bugName{"type mismatch"};
    std::string errorText{"Buffer type and specified MPI type do not match. "};

    llvm::SmallVector<SourceRange, 3> sourceRanges;
    sourceRanges.push_back(callRange);
    sourceRanges.push_back(callExpr->getArg(idxPair.first)->getSourceRange());
    sourceRanges.push_back(callExpr->getArg(idxPair.second)->getSourceRange());

    bugReporter_.EmitBasicReport(adc->getDecl(), &checkerBase_, bugName,
                                 MPIError, errorText, location, sourceRanges);
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
    std::string bugName{"collective call inside rank branch"};
    std::string errorText{
        "Collective calls must be executed by all processes."
        " Move this call out of the rank branch. "};

    bugReporter_.EmitBasicReport(adc->getDecl(), &checkerBase_, bugName,
                                 MPIError, errorText, location, range);
}

/**
 * Report unmatched call for point to point send/recv functions.
 *
 * @param callExpr
 * @param missingType
 */
void MPIBugReporter::reportUnmatchedCall(const CallExpr *const callExpr,
                                         std::string missingType) const {
    auto adc = analysisManager_.getAnalysisDeclContext(currentFunctionDecl_);
    PathDiagnosticLocation location = PathDiagnosticLocation::createBegin(
        callExpr, bugReporter_.getSourceManager(), adc);

    SourceRange range = callExpr->getCallee()->getSourceRange();
    std::string bugName{"unmatched point to point function"};
    std::string errorText{"No matching " + missingType + " function found. "};

    bugReporter_.EmitBasicReport(adc->getDecl(), &checkerBase_, bugName,
                                 MPIError, errorText, location, range);
}

/**
 * Report non-integer value usage at indices where not allowed.
 * (e.g. count, rank)
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
    std::string bugName{"invalid argument type"};
    std::string errorText{typeAsString + " type used at index " +
                          indexAsString + " is not valid. "};

    SmallVector<SourceRange, 3> sourceRanges;
    sourceRanges.push_back(callExprRange);
    sourceRanges.push_back(invalidSourceRange);
    sourceRanges.push_back(callExpr->getArg(idx)->getSourceRange());
    bugReporter_.EmitBasicReport(d->getDecl(), &checkerBase_, bugName, MPIError,
                                 errorText, location, sourceRanges);
}

// path sensitive reports –––––––––––––––––––––––––––––––––––––––––––––––––
/**
 * Report duplicate request use by nonblocking calls.
 *
 * @param observedCall
 * @param requestVar
 * @param node
 */
void MPIBugReporter::reportDoubleNonblocking(
    const CallEvent &observedCall, const RequestVar &requestVar,
    const ExplodedNode *const node) const {
    std::string lineNo{lineNumber(requestVar.lastUser_)};

    std::string lastUser =
        requestVar.lastUser_->getCalleeIdentifier()->getName();

    std::string errorText{"Request " + requestVar.variableName() +
                          " is already in use by nonblocking call " + lastUser +
                          " in line " + lineNo + ". "};

    auto bugReport = llvm::make_unique<BugReport>(*doubleNonblockingBugType_,
                                                  errorText, node);
    bugReport->addRange(observedCall.getSourceRange());
    bugReport->addRange(requestVar.lastUser_->getSourceRange());
    SourceRange r = util::sourceRange(requestVar.memRegion_);
    if (r.isValid()) bugReport->addRange(r);
    bugReporter_.emitReport(std::move(bugReport));
}

/**
 * Report duplicate request use by waits.
 *
 * @param observedCall
 * @param requestVar
 * @param node
 */
void MPIBugReporter::reportDoubleWait(const CallEvent &observedCall,
                                      const RequestVar &requestVar,
                                      const ExplodedNode *const node) const {
    std::string lineNo{lineNumber(requestVar.lastUser_)};
    std::string lastUser =
        requestVar.lastUser_->getCalleeIdentifier()->getName();
    std::string errorText{"Request " + requestVar.variableName() +
                          " is already waited upon by " + lastUser +
                          " in line " + lineNo + ". "};

    auto bugReport =
        llvm::make_unique<BugReport>(*doubleWaitBugType_, errorText, node);
    bugReport->addRange(observedCall.getSourceRange());
    bugReport->addRange(requestVar.lastUser_->getSourceRange());
    SourceRange r = util::sourceRange(requestVar.memRegion_);
    if (r.isValid()) bugReport->addRange(r);
    bugReporter_.emitReport(std::move(bugReport));
}

/**
 * Report a missing wait for a nonblocking call.
 *
 * @param requestVar
 * @param node
 */
void MPIBugReporter::reportMissingWait(const RequestVar &requestVar,
                                       const ExplodedNode *const node) const {
    std::string lineNo{lineNumber(requestVar.lastUser_)};
    std::string lastUser =
        requestVar.lastUser_->getCalleeIdentifier()->getName();

    std::string errorText{
        lastUser + " in line " + lineNo +
        ", using request " + requestVar.variableName() +
        ", has no matching wait in the scope of this function. "};

    auto bugReport =
        llvm::make_unique<BugReport>(*missingWaitBugType_, errorText, node);
    bugReport->addRange(requestVar.lastUser_->getSourceRange());
    SourceRange r = util::sourceRange(requestVar.memRegion_);
    if (r.isValid()) bugReport->addRange(r);
    bugReporter_.emitReport(std::move(bugReport));
}

/**
 * Report there's no matching nonblocking call for request var used by wait.
 *
 * @param callExpr
 * @param requestVar
 * @param node
 */
void MPIBugReporter::reportUnmatchedWait(
    const CallEvent &callEvent, const clang::ento::MemRegion *requestRegion,
    const ExplodedNode *const node) const {
    std::string errorText{"Request " + util::variableName(requestRegion) +
                          " has no matching nonblocking call. "};

    auto bugReport =
        llvm::make_unique<BugReport>(*unmatchedWaitBugType_, errorText, node);
    bugReport->addRange(callEvent.getSourceRange());
    SourceRange r = util::sourceRange(requestRegion);
    if (r.isValid()) bugReport->addRange(r);
    bugReporter_.emitReport(std::move(bugReport));
}

}  // end of namespace: mpi
