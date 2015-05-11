#ifndef MPICHECKERIMPL_HPP_O1KSUWZO
#define MPICHECKERIMPL_HPP_O1KSUWZO

#include <utility>
#include "../ClangSACheckers.h"
#include "clang/StaticAnalyzer/Core/Checker.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "llvm/ADT/SmallVector.h"
#include "clang/Lex/Lexer.h"

#include "MPIFunctionClassifier.hpp"
#include "MPIBugReporter.hpp"
#include "Container.hpp"
#include "Utility.hpp"

namespace mpi {

/**
 * Class to implement the actual checks.
 */
class MPICheckerImpl : public clang::RecursiveASTVisitor<MPICheckerImpl> {
public:
    MPICheckerImpl(clang::ento::BugReporter &bugReporter,
                   const clang::ento::CheckerBase &checkerBase,
                   clang::ento::AnalysisManager &analysisManager)
        : funcClassifier_{analysisManager},
          bugReporter_{bugReporter, checkerBase, analysisManager},
          analysisManager_{analysisManager} {}

    // validation functions
    bool areComponentsOfArgumentEqual(const MPICall &, const MPICall &,
                                      const size_t) const;
    bool areDatatypesEqual(const MPICall &, const MPICall &,
                           const size_t) const;
    bool areCommunicationTypesEqual(const MPICall &, const MPICall &) const;

    void checkForInvalidArgs(const MPICall &) const;

    void checkForRedundantCall(const MPICall &) const;
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

    void checkForRedundantCalls() const;
    void checkRequestUsage(const MPICall &) const;
    void checkForCollectiveInCase(const MPICall &) const;
    bool isSendRecvPair(const MPICall &, const MPICall &) const;

    MPIFunctionClassifier funcClassifier_;
    MPIBugReporter bugReporter_;
    clang::ento::AnalysisManager &analysisManager_;
};

}  // end of namespace: mpi

#endif  // end of include guard: MPICHECKERIMPL_HPP_O1KSUWZO
