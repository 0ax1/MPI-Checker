#ifndef MPISCHEMACHECKERAST_HPP_NKN9I06D
#define MPISCHEMACHECKERAST_HPP_NKN9I06D

#include "../ClangSACheckers.h"
#include "clang/StaticAnalyzer/Core/Checker.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"
#include "clang/AST/RecursiveASTVisitor.h"

#include <vector>
#include <string>

#include "SupportingVisitors.hpp"
#include "MPIFunctionClassifier.cpp"
#include "MPIBugReporter.hpp"
#include "MPITypes.hpp"

namespace mpi {

// forward declaration
struct MPICall;

// callback functions a checker can register for
// http://clang.llvm.org/doxygen/CheckerDocumentation_8cpp_source.html

// dump-color legend
// Red           - CastColor
// Green         - TypeColor
// Bold Green    - DeclKindNameColor, UndeserializedColor
// Yellow        - AddressColor, LocationColor
// Blue          - CommentColor, NullColor, IndentColor
// Bold Blue     - AttrColor
// Bold Magenta  - StmtColor
// Cyan          - ValueKindColor, ObjectKindColor
// Bold Cyan     - ValueColor, DeclNameColor

/**
 * Main visitor class to collect information about MPI calls traversing
 * the AST and checking invariants during the traversal.
 * This class uses several helper classes: MPIFunctionClassifier,
 * MPIBugreporter, additional visitors from 'SupportingVisitors.hpp'.
 *
 */
class MPIVisitor : public clang::RecursiveASTVisitor<MPIVisitor> {
private:
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
                           const StringRef,
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

    llvm::SmallVectorImpl<mpi::ExprVisitor> &&argumentBuilder(
        clang::CallExpr *) const;

    MPIFunctionClassifier funcClassifier_;
    MPIBugReporter bugReporter_;
    clang::ento::AnalysisManager &analysisManager_;

public:
    enum class MatchType { kMatch, kMismatch, kNoMatch };

    MPIVisitor(clang::ento::BugReporter &bugReporter,
               const clang::ento::CheckerBase &checkerBase,
               clang::ento::AnalysisManager &analysisManager)
        : funcClassifier_{analysisManager},
          bugReporter_{bugReporter, checkerBase, analysisManager},
          analysisManager_{analysisManager} {}

    // visitor callbacks
    bool VisitFunctionDecl(clang::FunctionDecl *);
    bool VisitCallExpr(clang::CallExpr *);
    bool VisitIfStmt(clang::IfStmt *);

    void checkForRedundantCalls() const;
    void checkRequestUsage(const MPICall &) const;
    void trackRankVariables(const MPICall &) const;
};

}  // end of namespace: mpi

#endif  // end of include guard: MPISCHEMACHECKERAST_HPP_NKN9I06D
