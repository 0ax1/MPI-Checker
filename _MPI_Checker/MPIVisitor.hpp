#ifndef MPISCHEMACHECKERAST_HPP_NKN9I06D
#define MPISCHEMACHECKERAST_HPP_NKN9I06D

#include "MPICheckerAST.hpp"

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
 * through MPICheckerAST.
 *
 */
class MPIVisitor : public clang::RecursiveASTVisitor<MPIVisitor> {
public:
    MPIVisitor(clang::ento::BugReporter &bugReporter,
               const clang::ento::CheckerBase &checkerBase,
               clang::ento::AnalysisManager &analysisManager)
        : checkerAST_{bugReporter, checkerBase, analysisManager} {}

    // visitor callbacks
    bool VisitFunctionDecl(clang::FunctionDecl *);
    bool VisitCallExpr(clang::CallExpr *);
    bool VisitIfStmt(clang::IfStmt *);

    MPICheckerAST checkerAST_;

private:
    bool isRankBranch(clang::IfStmt *ifStmt);
    MPIRankCase buildRankCase(clang::Stmt *, clang::Stmt *,
                              llvm::SmallVector<clang::Stmt *, 4>);

    llvm::SmallVector<clang::IfStmt *, 8> visitedIfStmts_;
};

}  // end of namespace: mpi

#endif  // end of include guard: MPISCHEMACHECKERAST_HPP_NKN9I06D
