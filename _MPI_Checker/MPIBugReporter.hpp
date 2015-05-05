#ifndef MPIBUGREPORTER_HPP_57XZJI4L
#define MPIBUGREPORTER_HPP_57XZJI4L

#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"
#include "Typedefs.hpp"

namespace mpi {

class MPIBugReporter {
private:
    clang::ento::BugReporter &bugReporter_;
    const clang::ento::CheckerBase &checkerBase_;
    clang::ento::AnalysisManager &analysisManager_;

public:
    MPIBugReporter(clang::ento::BugReporter &bugReporter,
                   const clang::ento::CheckerBase &checkerBase,
                   clang::ento::AnalysisManager &analysisManager)
        : bugReporter_{bugReporter},
          checkerBase_{checkerBase},
          analysisManager_{analysisManager} {}

    void reportTypeMismatch(clang::CallExpr *) const;
    void reportInvalidArgumentType(clang::CallExpr *, size_t,
                                   InvalidArgType) const;
    void reportDuplicate(const clang::CallExpr *,
                         const clang::CallExpr *) const;

    clang::Decl *currentFunctionDecl_{nullptr};
};

}  // end of namespace: mpi

#endif  // end of include guard: MPIBUGREPORTER_HPP_57XZJI4L
