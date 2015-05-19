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
        DoubleWaitBugType.reset(
            new clang::ento::BugType(&checkerBase, "double wait", "MPI Error"));
        UnmatchedWaitBugType.reset(new clang::ento::BugType(
            &checkerBase, "unmatched wait", "MPI Error"));
        DoubleNonblockingBugType.reset(new clang::ento::BugType(
            &checkerBase, "double request usage", "MPI Error"));
        MissingWaitBugType.reset(new clang::ento::BugType(
            &checkerBase, "missing wait", "MPI Error"));
    }

    // ast reports ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
    void reportTypeMismatch(const clang::CallExpr *const,
                            const std::pair<size_t, size_t> &) const;
    void reportInvalidArgumentType(const clang::CallExpr *const, const size_t,
                                   const clang::SourceRange,
                                   const std::string &) const;
    void reportRedundantCall(const clang::CallExpr *const,
                             const clang::CallExpr *const,
                             const llvm::SmallVectorImpl<size_t> &) const;

    void reportCollCallInBranch(const clang::CallExpr *const) const;
    void reportUnmatchedCall(const clang::CallExpr *const, std::string) const;
    void reportNotReachableCall(const clang::CallExpr *const) const;

    // path sensitive reports –––––––––––––––––––––––––––––––––––––––––––––––
    void reportMissingWait(const RequestVar &, clang::ento::ExplodedNode *) const;

    void reportUnmatchedWait(const clang::CallExpr *,
                             const clang::VarDecl *requestVar,
                             clang::ento::ExplodedNode *) const;

    void reportDoubleWait(const clang::CallExpr *, const RequestVar &,
                          clang::ento::ExplodedNode *) const;

    void reportDoubleNonblocking(const clang::CallExpr *, const RequestVar &,
                                 clang::ento::ExplodedNode *) const;

    clang::Decl *currentFunctionDecl_{nullptr};

private:
    std::string lineNumberForCallExpr(const clang::CallExpr *) const;

    std::unique_ptr<clang::ento::BugType> UnmatchedWaitBugType;
    std::unique_ptr<clang::ento::BugType> MissingWaitBugType;
    std::unique_ptr<clang::ento::BugType> DoubleWaitBugType;
    std::unique_ptr<clang::ento::BugType> DoubleNonblockingBugType;

    clang::ento::BugReporter &bugReporter_;
    const clang::ento::CheckerBase &checkerBase_;
    clang::ento::AnalysisManager &analysisManager_;
};

}  // end of namespace: mpi
#endif  // end of include guard: MPIBUGREPORTER_HPP_57XZJI4L
