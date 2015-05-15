#ifndef MPICHECKERSENS_HPP_BKYOQUPL
#define MPICHECKERSENS_HPP_BKYOQUPL

#include "clang/StaticAnalyzer/Core/BugReporter/BugType.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugReporter.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"
#include "MPIFunctionClassifier.hpp"
#include "MPITypes.hpp"
#include "MPIBugReporter.hpp"

namespace mpi {

class MPICheckerSens {
public:
    MPICheckerSens(clang::ento::AnalysisManager &analysisManager,
                   const clang::ento::CheckerBase *checkerBase,
                   clang::ento::BugReporter &bugReporter)
        : funcClassifier_{analysisManager},
          bugReporter_{bugReporter, *checkerBase, analysisManager} {}

    void checkDoubleNonblocking(const clang::CallExpr *,
                                clang::ento::CheckerContext &) const;
    void checkWaitUsage(const clang::CallExpr *,
                        clang::ento::CheckerContext &) const;
    void checkMissingWait(clang::ento::CheckerContext &);
    void clearRankVars(clang::ento::CheckerContext &) const;

private:
    MPIFunctionClassifier funcClassifier_;
    MPIBugReporter bugReporter_;
};
}  // end of namespace: mpi

#endif  // end of include guard: MPICHECKERSENS_HPP_BKYOQUPL
