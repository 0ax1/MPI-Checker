#ifndef MPIVISITOR_CPP_H6L3JFDT
#define MPIVISITOR_CPP_H6L3JFDT

#include <functional>
#include "MPIVisitor.hpp"
#include "llvm/ADT/SmallVector.h"

using namespace clang;
using namespace ento;

namespace mpi {

// visitor functions –––––––––––––––––––––––––––––––––––––––––––––––––––––
bool MPIVisitor::VisitFunctionDecl(FunctionDecl *functionDecl) {
    // to keep track which function implementation is currently analysed
    if (functionDecl->clang::Decl::hasBody() && !functionDecl->isInlined()) {
        // to make display of function in diagnostics available
        checker_.bugReporter_.currentFunctionDecl_ = functionDecl;
    }
    return true;
}

bool MPIVisitor::isRankBranch(clang::IfStmt *ifStmt) {
    bool isInRankBranch{false};
    ExprVisitor exprVisitor{ifStmt->getCond()};
    for (const VarDecl *const varDecl : exprVisitor.vars_) {
        if (cont::isContained(MPIRank::visitedRankVariables, varDecl) ||
                varDecl->getNameAsString() == "rank") {
            isInRankBranch = true;
            break;
        }
    }
    return isInRankBranch;
}

/**
 * Visits rankCases. Checks if a rank variable is involved.
 *
 * @param ifStmt
 *
 * @return
 */
bool MPIVisitor::VisitIfStmt(IfStmt *ifStmt) {
    if (!isRankBranch(ifStmt)) return true;  // only inspect rank branches

    // vector element must be assignable -> use std:vector is inside
    llvm::SmallVector<MPIrankCase, 4> rankCases;

    llvm::SmallVector<Stmt *, 4> unmatchedCases;
    unmatchedCases.push_back(ifStmt->getCond());

    // collect mpi calls in if
    rankCases.emplace_back(
        collectMPICallsInCase(ifStmt->getThen(), ifStmt->getCond(), {}));

    // collect mpi calls in all else if
    Stmt *elseStmt = ifStmt->getElse();
    while (IfStmt *elseIf = dyn_cast_or_null<IfStmt>(elseStmt)) {
        rankCases.emplace_back(
            collectMPICallsInCase(elseIf->getThen(), elseIf->getCond(), {}));
        unmatchedCases.push_back(elseIf->getCond());
        elseStmt = elseIf->getElse();
    }

    // collect mpi calls in else
    if (elseStmt)
        rankCases.emplace_back(
            collectMPICallsInCase(elseStmt, nullptr, unmatchedCases));

    // copy rank cases for complete translation unit analysis
    cont::copy(rankCases, MPIRankCases::visitedRankCases);

    // check if collective calls are used in rank rankCase
    for (const MPIrankCase &rankCase : rankCases) {
        for (const MPICall &call : rankCase) {
            checker_.checkForCollectiveCall(call);
        }
    }

    return true;
}

/**
 * Visited for each function call.
 *
 * @param callExpr
 *
 * @return
 */
bool MPIVisitor::VisitCallExpr(CallExpr *callExpr) {
    const FunctionDecl *functionDecl = callExpr->getDirectCallee();

    if (checker_.funcClassifier_.isMPIType(functionDecl->getIdentifier())) {
        MPICall mpiCall{callExpr};

        checker_.checkBufferTypeMatch(mpiCall);
        checker_.checkForInvalidArgs(mpiCall);
        trackRankVariables(mpiCall);

        if (checker_.funcClassifier_.isCollectiveType(mpiCall)) {
            MPICall::visitedCalls.push_back(std::move(mpiCall));
        }
        checker_.checkRequestUsage(mpiCall);
    }

    return true;
}

void MPIVisitor::trackRankVariables(const MPICall &mpiCall) const {
    if (checker_.funcClassifier_.isMPI_Comm_rank(mpiCall)) {
        VarDecl *varDecl = mpiCall.arguments_[1].vars_[0];
        MPIRank::visitedRankVariables.insert(varDecl);
    }
}

MPIrankCase MPIVisitor::collectMPICallsInCase(
    Stmt *then, Stmt *condition, llvm::SmallVector<Stmt *, 4> unmatchedCases) {
    MPIrankCase rankCaseVector;
    StmtVisitor stmtVisitor{then};  // collect call exprs
    for (CallExpr *callExpr : stmtVisitor.callExprs_) {
        // filter mpi calls
        if (checker_.funcClassifier_.isMPIType(
                callExpr->getDirectCallee()->getIdentifier())) {
            if (unmatchedCases.size()) {
                MPICall::visitedCalls.emplace_back(callExpr, condition,
                                                   unmatchedCases);
            } else {
                MPICall::visitedCalls.emplace_back(callExpr, condition);
            }

            // add reference to rankCase vector
            rankCaseVector.push_back(MPICall::visitedCalls.back());
        }
    }
    return rankCaseVector;
}


// host class ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
/**
 * Checker host class. Registers checker functionality.
 * Class name determines checker name to specify when the command line
 * is invoked for static analysis.
 * Receives callback for every translation unit about to visit.
 */
class MPIChecker : public Checker<check::ASTDecl<TranslationUnitDecl>> {
public:
    void checkASTDecl(const TranslationUnitDecl *tuDecl,
                      AnalysisManager &analysisManager,
                      BugReporter &bugReporter) const {
        MPIVisitor visitor{bugReporter, *this, analysisManager};
        visitor.TraverseTranslationUnitDecl(
            const_cast<TranslationUnitDecl *>(tuDecl));

        // invoked after travering the translation unit
        // visitor.checker_.checkForRedundantCalls();
        visitor.checker_.checkPointToPointSchema();


        // clear after every translation unit
        MPICall::visitedCalls.clear();
        MPIRequest::visitedRequests.clear();
        MPIRankCases::visitedRankCases.clear();
    }
};

}  // end of namespace: mpi

void ento::registerMPIChecker(CheckerManager &mgr) {
    mgr.registerChecker<mpi::MPIChecker>();
}

#endif  // end of include guard: MPIVISITOR_CPP_H6L3JFDT
