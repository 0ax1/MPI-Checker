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
            const clang::ento::CheckerBase * checkerBase)
        : funcClassifier_{analysisManager}, checkerBase_{checkerBase} {
        initBugTypes();
    }

    void initBugTypes();
    void checkNonBlockingUsage(const clang::CallExpr *,
                          clang::ento::CheckerContext &) const;
    void checkWaitUsage(const clang::CallExpr *,
                   clang::ento::CheckerContext &) const;
    void checkForUnmatchedWait(clang::ento::CheckerContext &);
    void clearRankVars(clang::ento::CheckerContext &) const;

private:
    std::unique_ptr<clang::ento::BugType> UnmatchedWaitBugType{nullptr};
    std::unique_ptr<clang::ento::BugType> MissingWaitBugType{nullptr};
    std::unique_ptr<clang::ento::BugType> DoubleWaitBugType{nullptr};
    std::unique_ptr<clang::ento::BugType> DoubleRequestBugType{nullptr};
    MPIFunctionClassifier funcClassifier_;
    const clang::ento::CheckerBase *const checkerBase_;
};

}  // end of namespace: mpi

#endif  // end of include guard: MPICHECKERSENS_HPP_BKYOQUPL
