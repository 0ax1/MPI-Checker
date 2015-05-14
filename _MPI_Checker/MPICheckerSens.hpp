#ifndef MPICHECKERSENS_HPP_BKYOQUPL
#define MPICHECKERSENS_HPP_BKYOQUPL

#include "MPIFunctionClassifier.hpp"
#include "MPITypes.hpp"
#include "clang/StaticAnalyzer/Core/BugReporter/BugType.h"
#include <memory>

namespace mpi {

class MPICheckerSens {
public:
    MPICheckerSens(clang::ento::AnalysisManager &analysisManager,
                   const clang::ento::CheckerBase *checkerBase)
        : checkerBase_{checkerBase}, funcClassifier_{analysisManager} {
    }

    void checkDoubleNonblocking(const clang::CallExpr *,
                               clang::ento::CheckerContext &) const;
    void checkWaitUsage(const clang::CallExpr *,
                        clang::ento::CheckerContext &) const;
    void checkMissingWait(clang::ento::CheckerContext &);
    void clearRankVars(clang::ento::CheckerContext &) const;

    mutable const clang::ento::CheckerBase *checkerBase_;

private:
    MPIFunctionClassifier funcClassifier_;
};
}  // end of namespace: mpi

#endif  // end of include guard: MPICHECKERSENS_HPP_BKYOQUPL
