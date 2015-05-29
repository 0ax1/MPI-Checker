#include "TranslationUnitVisitor.hpp"
#include "MPICheckerPathSensitive.hpp"

using namespace clang;
using namespace ento;

namespace mpi {

/**
 * Visited for each appearing function declaration.
 *
 * @param functionDecl
 *
 * @return continue visiting
 */
bool TranslationUnitVisitor::VisitFunctionDecl(FunctionDecl *functionDecl) {
    // to keep track which function implementation is currently analysed
    if (functionDecl->clang::Decl::hasBody() && !functionDecl->isInlined()) {
        // to make display of function in diagnostics available
        checkerAST_.setCurrentlyVisitedFunction(functionDecl);
    }
    return true;
}

/**
 * Visits rankCases. Checks if a rank variable is involved.
 * Visits all if and else if!
 *
 * @param ifStmt
 *
 * @return continue visiting
 */
bool TranslationUnitVisitor::VisitIfStmt(IfStmt *ifStmt) {
    if (!isRankBranch(ifStmt)) return true;  // only inspect rank branches
    if (cont::isContained(visitedIfStmts_, ifStmt)) return true;

    std::vector<ConditionVisitor> unmatchedConditions;

    // collect mpi calls in if / else if
    Stmt *stmt = ifStmt;
    while (IfStmt *ifStmt = dyn_cast_or_null<IfStmt>(stmt)) {
        MPIRankCase::visitedRankCases.emplace_back(
            ifStmt->getThen(), ifStmt->getCond(), unmatchedConditions,
            checkerAST_.funcClassifier());
        unmatchedConditions.push_back(ifStmt->getCond());
        stmt = ifStmt->getElse();
        visitedIfStmts_.push_back(ifStmt);
        checkerAST_.checkForCollectiveCalls(
            MPIRankCase::visitedRankCases.back());
    }

    // collect mpi calls in else
    if (stmt) {
        MPIRankCase::visitedRankCases.emplace_back(
            stmt, nullptr, unmatchedConditions, checkerAST_.funcClassifier());
        checkerAST_.checkForCollectiveCalls(
            MPIRankCase::visitedRankCases.back());
    }

    return true;
}

/**
 * Visited for each function call.
 *
 * @param callExpr
 *
 * @return continue visiting
 */
bool TranslationUnitVisitor::VisitCallExpr(CallExpr *callExpr) {
    const FunctionDecl *functionDecl = callExpr->getDirectCallee();

    if (checkerAST_.funcClassifier().isMPIType(functionDecl->getIdentifier())) {
        MPICall mpiCall{callExpr};

        checkerAST_.checkBufferTypeMatch(mpiCall);
        checkerAST_.checkForInvalidArgs(mpiCall);
    }

    return true;
}

/**
 * Checks if a rank variable is used in branch condition.
 *
 * @param ifStmt
 *
 * @return if rank var is used
 */
bool TranslationUnitVisitor::isRankBranch(clang::IfStmt *ifStmt) {
    bool isInRankBranch{false};
    ConditionVisitor ConditionVisitor{ifStmt->getCond()};
    for (const VarDecl *const varDecl : ConditionVisitor.vars()) {
        if (cont::isContained(MPIRank::visitedRankVariables, varDecl)) {
            isInRankBranch = true;
            break;
        }
    }
    return isInRankBranch;
}


}  // end of namespace: mpi
