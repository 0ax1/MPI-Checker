#ifndef MPIBUGREPORTER_HPP_57XZJI4L
#define MPIBUGREPORTER_HPP_57XZJI4L

#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"
#include "Typedefs.hpp"

namespace mpi {

class MPIBugReporter {
public:
    MPIBugReporter(clang::ento::BugReporter &bugReporter,
                   const clang::ento::CheckerBase &checkerBase,
                   clang::ento::AnalysisManager &analysisManager)
        : bugReporter_{bugReporter},
          checkerBase_{checkerBase},
          analysisManager_{analysisManager} {}

    void reportTypeMismatch(const clang::CallExpr *,
                            const std::pair<size_t, size_t> &) const;
    void reportInvalidArgumentType(clang::CallExpr *, size_t,
                                   clang::SourceRange, InvalidArgType) const;
    void reportRedundantCall(const clang::CallExpr *, const clang::CallExpr *,
                             const llvm::SmallVectorImpl<size_t> &) const;
    void reportDoubleRequestUse(const clang::CallExpr *, const clang::VarDecl *,
                                const clang::CallExpr *) const;
    void reportUnmatchedWait(const clang::CallExpr *,
                             const clang::VarDecl *) const;

    clang::Decl *currentFunctionDecl_{nullptr};

private:
    std::string lineNumberForCallExpr(const clang::CallExpr *) const;

    clang::ento::BugReporter &bugReporter_;
    const clang::ento::CheckerBase &checkerBase_;
    clang::ento::AnalysisManager &analysisManager_;
};

}  // end of namespace: mpi
#endif  // end of include guard: MPIBUGREPORTER_HPP_57XZJI4L
