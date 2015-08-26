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

#ifndef MPIBUGREPORTER_HPP_57XZJI4L
#define MPIBUGREPORTER_HPP_57XZJI4L

#include "clang/StaticAnalyzer/Core/BugReporter/BugType.h"
#include "MPITypes.hpp"

namespace mpi {

/**
 * Emits bug reports for ast and path sensitive checks.
 */
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
    void reportInvalidArgumentType(const clang::CallExpr *const,
                                   const size_t) const;
    void reportRedundantCall(const clang::CallExpr *const,
                             const clang::CallExpr *const) const;

    void reportCollCallInBranch(const clang::CallExpr *const) const;
    void reportUnmatchedCall(const clang::CallExpr *const, std::string) const;
    void reportNotReachableCall(const clang::CallExpr *const) const;

    // path sensitive reports –––––––––––––––––––––––––––––––––––––––––––––––
    void reportMissingWait(const RequestVar &,
                           const clang::ento::ExplodedNode *const) const;

    void reportUnmatchedWait(const clang::ento::CallEvent &,
                             const clang::ento::MemRegion *,
                             const clang::ento::ExplodedNode *const) const;

    void reportDoubleWait(const clang::ento::CallEvent &, const RequestVar &,
                          const clang::ento::ExplodedNode *const) const;

    void reportDoubleNonblocking(const clang::ento::CallEvent &,
                                 const mpi::RequestVar &,
                                 const clang::ento::ExplodedNode *const) const;

    const clang::Decl *currentFunctionDecl_{nullptr};

private:
    std::string lineNumber(const clang::ento::CallEventRef<>) const;

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
