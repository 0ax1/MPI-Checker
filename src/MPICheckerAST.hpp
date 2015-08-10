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

#ifndef MPICHECKERAST_HPP_O1KSUWZO
#define MPICHECKERAST_HPP_O1KSUWZO

#include "../../ClangSACheckers.h"
#include "MPIFunctionClassifier.hpp"
#include "MPIBugReporter.hpp"
#include "Container.hpp"
#include "Utility.hpp"
#include "TypeVisitor.hpp"

namespace mpi {

/**
 * Class to check the mpi schema based on AST information.
 */
class MPICheckerAST : public clang::RecursiveASTVisitor<MPICheckerAST> {
public:
    MPICheckerAST(clang::ento::BugReporter &bugReporter,
                  const clang::ento::CheckerBase &checkerBase,
                  clang::ento::AnalysisManager &analysisManager)
        : funcClassifier_{analysisManager},
          bugReporter_{bugReporter, checkerBase, analysisManager},
          analysisManager_{analysisManager} {}

    using IndexPairs = llvm::SmallVector<std::pair<size_t, size_t>, 2>;

    void checkPointToPointSchema() const;
    void checkReachbility() const;
    void checkForRedundantCalls() const;
    void checkForCollectiveCalls(const MPIRankCase &) const;
    void checkForInvalidArgs(const MPICall &) const;
    void checkBufferTypeMatch(const MPICall &mpiCall) const;
    IndexPairs bufferDataTypeIndices(const MPICall &) const;
    void setCurrentlyVisitedFunction(const clang::FunctionDecl *const);
    const MPIFunctionClassifier &funcClassifier() { return funcClassifier_; }

private:
    bool isSendRecvPair(const MPICall &, const MPICall &, const MPIRankCase &,
                        const MPIRankCase &) const;
    bool areDatatypesEqual(const MPICall &, const MPICall &) const;
    void checkUnmatchedCalls() const;
    void checkSendRecvMatches(const MPIRankCase &, const MPIRankCase &) const;
    void checkReachbilityPair(const MPIRankCase &, const MPIRankCase &) const;
    llvm::SmallVector<size_t, 1> integerIndices(const MPICall &) const;

    void selectTypeMatcher(const TypeVisitor &, const MPICall &,
                           const clang::StringRef,
                           const std::pair<size_t, size_t> &) const;
    bool matchBoolType(const TypeVisitor &, const llvm::StringRef) const;
    bool matchCharType(const TypeVisitor &, const llvm::StringRef) const;
    bool matchSignedType(const TypeVisitor &, const llvm::StringRef) const;
    bool matchUnsignedType(const TypeVisitor &, const llvm::StringRef) const;
    bool matchFloatType(const TypeVisitor &, const llvm::StringRef) const;
    bool matchComplexType(const TypeVisitor &, const llvm::StringRef) const;
    bool matchExactWidthType(const TypeVisitor &, const llvm::StringRef) const;

    MPIFunctionClassifier funcClassifier_;
    MPIBugReporter bugReporter_;
    clang::ento::AnalysisManager &analysisManager_;
};

}  // end of namespace: mpi

#endif  // end of include guard: MPICHECKERAST_HPP_O1KSUWZO
