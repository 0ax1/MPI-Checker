#ifndef MPICHECKERAST_HPP_O1KSUWZO
#define MPICHECKERAST_HPP_O1KSUWZO

#include "../ClangSACheckers.h"
#include "MPIFunctionClassifier.hpp"
#include "MPIBugReporter.hpp"
#include "Container.hpp"
#include "Utility.hpp"
#include "TypeVisitor.hpp"

namespace mpi {

/**
 * Class to implement the actual checks.
 */
class MPICheckerAST : public clang::RecursiveASTVisitor<MPICheckerAST> {
public:
    MPICheckerAST(clang::ento::BugReporter &bugReporter,
                  const clang::ento::CheckerBase &checkerBase,
                  clang::ento::AnalysisManager &analysisManager)
        : funcClassifier_{analysisManager},
          bugReporter_{bugReporter, checkerBase, analysisManager},
          analysisManager_{analysisManager} {}

    void checkPointToPointSchema() const;
    void checkReachbility() const;
    void checkForRedundantCalls() const;
    void checkForCollectiveCalls(const MPIRankCase &) const;
    void checkForInvalidArgs(const MPICall &) const;
    void checkBufferTypeMatch(const MPICall &mpiCall) const;
    using IndexPairs = llvm::SmallVector<std::pair<size_t, size_t>, 2>;
    IndexPairs bufferDataTypeIndices(const MPICall &) const;
    void setCurrentlyVisitedFunction(
        const clang::FunctionDecl *const functionDecl) {
        bugReporter_.currentFunctionDecl_ = functionDecl;
    }
    const MPIFunctionClassifier &funcClassifier() { return funcClassifier_; }

private:
    bool isSendRecvPair(const MPICall &, const MPICall &) const;
    bool areDatatypesEqual(const MPICall &, const MPICall &) const;
    void checkUnmatchedCalls() const;
    void checkSendRecvMatches(const MPIRankCase &, const MPIRankCase &) const;
    void checkReachbilityPair(const MPIRankCase &, const MPIRankCase &) const;
    void checkForRedundantCall(const MPICall &callToCheck,
                               const MPIRankCase &) const;
    bool qualifyRedundancyCheck(const MPICall &, const MPICall &) const;
    std::vector<size_t> integerIndices(const MPICall &) const;

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
