#ifndef MPICheckerPathSensitive_HPP_BKYOQUPL
#define MPICheckerPathSensitive_HPP_BKYOQUPL

#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"
#include "MPIFunctionClassifier.hpp"
#include "MPITypes.hpp"
#include "MPIBugReporter.hpp"

namespace mpi {

class MPICheckerPathSensitive {
public:
    MPICheckerPathSensitive(clang::ento::AnalysisManager &analysisManager,
                            const clang::ento::CheckerBase *checkerBase,
                            clang::ento::BugReporter &bugReporter)
        : funcClassifier_{analysisManager},
          bugReporter_{bugReporter, *checkerBase, analysisManager} {}

    void checkDoubleNonblocking(const clang::CallExpr *,
                                clang::ento::CheckerContext &) const;
    void checkWaitUsage(const clang::CallExpr *,
                        clang::ento::CheckerContext &) const;
    void checkMissingWaits(clang::ento::CheckerContext &);
    void clearRequestVars(clang::ento::CheckerContext &) const;

private:
    MPIFunctionClassifier funcClassifier_;
    MPIBugReporter bugReporter_;
};
}  // end of namespace: mpi

#endif  // end of include guard: MPICheckerPathSensitive_HPP_BKYOQUPL
