#ifndef MPICheckerAST_HPP_O1KSUWZO
#define MPICheckerAST_HPP_O1KSUWZO

#include "../ClangSACheckers.h"
#include "MPIFunctionClassifier.hpp"
#include "MPIBugReporter.hpp"
#include "Container.hpp"
#include "Utility.hpp"

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

    // validation functions
    void checkForInvalidArgs(const MPICall &) const;

    void checkForRedundantCall(const MPICall &callToCheck,
                               const MPIRankCase &rankCase) const;
    void checkForRedundantCalls() const;
    bool qualifyRedundancyCheck(const MPICall &, const MPICall &) const;

    void checkBufferTypeMatch(const MPICall &mpiCall) const;
    void selectTypeMatcher(const mpi::TypeVisitor &, const MPICall &,
                           const clang::StringRef,
                           const std::pair<size_t, size_t> &) const;
    bool matchBoolType(const mpi::TypeVisitor &, const llvm::StringRef) const;
    bool matchCharType(const mpi::TypeVisitor &, const llvm::StringRef) const;
    bool matchSignedType(const mpi::TypeVisitor &, const llvm::StringRef) const;
    bool matchUnsignedType(const mpi::TypeVisitor &,
                           const llvm::StringRef) const;
    bool matchFloatType(const mpi::TypeVisitor &, const llvm::StringRef) const;
    bool matchComplexType(const mpi::TypeVisitor &,
                          const llvm::StringRef) const;
    bool matchExactWidthType(const mpi::TypeVisitor &,
                             const llvm::StringRef) const;

    void checkForCollectiveCall(const MPICall &) const;
    bool isSendRecvPair(const MPICall &, const MPICall &) const;

    void checkUnmatchedCalls() const;
    void checkPointToPointSchema();
    void checkSendRecvMatches(const MPIRankCase &, const MPIRankCase &);
    void checkReachbility();
    void checkReachbilityPair(const MPIRankCase &, const MPIRankCase &);
    void unmarkCalls();

    MPIFunctionClassifier funcClassifier_;
    MPIBugReporter bugReporter_;
    clang::ento::AnalysisManager &analysisManager_;
};

}  // end of namespace: mpi

#endif  // end of include guard: MPICheckerAST_HPP_O1KSUWZO
