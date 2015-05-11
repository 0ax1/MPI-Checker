#ifndef MPISCHEMACHECKERAST_HPP_NKN9I06D
#define MPISCHEMACHECKERAST_HPP_NKN9I06D

#include "MPICheckerImpl.hpp"

namespace mpi {

// ast dump-color legend
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
 * the AST of a translation unit, checking invariants during the traversal
 * through MPICheckerImpl.
 *
 */
class MPIVisitor : public clang::RecursiveASTVisitor<MPIVisitor> {
public:
    enum class MatchType { kMatch, kMismatch, kNoMatch };

    MPIVisitor(clang::ento::BugReporter &bugReporter,
               const clang::ento::CheckerBase &checkerBase,
               clang::ento::AnalysisManager &analysisManager)
        : checker_{bugReporter, checkerBase, analysisManager} {}

    // visitor callbacks
    bool VisitFunctionDecl(clang::FunctionDecl *);
    bool VisitCallExpr(clang::CallExpr *);
    bool VisitIfStmt(clang::IfStmt *);

    void trackRankVariables(const MPICall &) const;
    MPICheckerImpl checker_;

private:
    bool isRankBranch(clang::IfStmt *ifStmt);
    std::vector<std::reference_wrapper<MPICall>> collectMPICallsInCase(
        clang::Stmt *, clang::Stmt *);
    void stripMatchingCalls(MPIrankCase &, MPIrankCase &);
};

}  // end of namespace: mpi

#endif  // end of include guard: MPISCHEMACHECKERAST_HPP_NKN9I06D
