#ifndef MPIBUGREPORTER_HPP_57XZJI4L
#define MPIBUGREPORTER_HPP_57XZJI4L

#include "clang/StaticAnalyzer/Core/BugReporter/BugType.h"
#include "MPITypes.hpp"

namespace mpi {

class MPIBugReporter {
public:
    MPIBugReporter(clang::ento::BugReporter &bugReporter,
                   const clang::ento::CheckerBase &checkerBase,
                   clang::ento::AnalysisManager &analysisManager)
        : bugReporter_{bugReporter},
          checkerBase_{checkerBase},
          analysisManager_{analysisManager} {
        doubleWaitBugType_.reset(
            new clang::ento::BugType(&checkerBase, "double wait", "MPI Error"));
        unmatchedWaitBugType_.reset(new clang::ento::BugType(
            &checkerBase, "unmatched wait", "MPI Error"));
        doubleNonblockingBugType_.reset(new clang::ento::BugType(
            &checkerBase, "double request usage", "MPI Error"));
        missingWaitBugType_.reset(new clang::ento::BugType(
            &checkerBase, "missing wait", "MPI Error"));
    }

    // ast reports ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
    void reportTypeMismatch(const clang::CallExpr *const,
                            const std::pair<size_t, size_t> &) const;
    void reportInvalidArgumentType(const clang::CallExpr *const, const size_t,
                                   const clang::SourceRange,
                                   const std::string &) const;
    void reportRedundantCall(const clang::CallExpr *const,
                             const clang::CallExpr *const) const;

    void reportCollCallInBranch(const clang::CallExpr *const) const;
    void reportUnmatchedCall(const clang::CallExpr *const, std::string) const;
    void reportNotReachableCall(const clang::CallExpr *const) const;

    // path sensitive reports –––––––––––––––––––––––––––––––––––––––––––––––
    void reportMissingWait(const RequestVar &,
                           const clang::ento::ExplodedNode *const) const;

    void reportUnmatchedWait(const clang::CallExpr *const,
                             const clang::VarDecl *const,
                             const clang::ento::ExplodedNode *const) const;

    void reportDoubleWait(const clang::CallExpr *, const RequestVar &,
                          const clang::ento::ExplodedNode *const) const;

    void reportDoubleNonblocking(const clang::CallExpr *const,
                                 const RequestVar &,
                                 const clang::ento::ExplodedNode *const) const;

    const clang::Decl *currentFunctionDecl_{nullptr};

private:
    std::string lineNumberForCallExpr(const clang::CallExpr *const) const;

    // path sensitive bug types
    std::unique_ptr<clang::ento::BugType> unmatchedWaitBugType_;
    std::unique_ptr<clang::ento::BugType> missingWaitBugType_;
    std::unique_ptr<clang::ento::BugType> doubleWaitBugType_;
    std::unique_ptr<clang::ento::BugType> doubleNonblockingBugType_;

    clang::ento::BugReporter &bugReporter_;
    const clang::ento::CheckerBase &checkerBase_;
    clang::ento::AnalysisManager &analysisManager_;
};

}  // end of namespace: mpi
#endif  // end of include guard: MPIBUGREPORTER_HPP_57XZJI4L
