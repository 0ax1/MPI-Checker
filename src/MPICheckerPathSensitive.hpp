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

#ifndef MPICHECKERPATHSENSITIVE_HPP_BKYOQUPL
#define MPICHECKERPATHSENSITIVE_HPP_BKYOQUPL

#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CallEvent.h"
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

    void checkDoubleNonblocking(const clang::ento::CallEvent &,
                                clang::ento::CheckerContext &) const;
    void checkWaitUsage(const clang::ento::CallEvent &,
                        clang::ento::CheckerContext &) const;
    void checkMissingWaits(clang::ento::CheckerContext &);
    void clearRequestVars(clang::ento::CheckerContext &) const;

private:
    const clang::ento::MemRegion *memRegionUsedInWait(
        const clang::ento::CallEvent &) const;

    void collectUsedMemRegions(
        llvm::SmallVector<const clang::ento::MemRegion *, 2> &,
        const clang::ento::MemRegion *, const clang::ento::CallEvent &,
        clang::ento::CheckerContext &) const;

    MPIFunctionClassifier funcClassifier_;
    MPIBugReporter bugReporter_;
};
}  // end of namespace: mpi
// TODO track request assignments

#endif  // end of include guard: MPICHECKERPATHSENSITIVE_HPP_BKYOQUPL
