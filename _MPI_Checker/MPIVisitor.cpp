#ifndef MPIVISITOR_CPP_H6L3JFDT
#define MPIVISITOR_CPP_H6L3JFDT

#include <functional>
#include "MPIVisitor.hpp"
#include "llvm/ADT/SmallVector.h"

using namespace clang;
using namespace ento;

namespace mpi {

// TODO deadlock detection
// TODO send/recv pair match

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
        if (cont::isContained(MPIRank::visitedRankVariables, varDecl)) {
            isInRankBranch = true;
            break;
        }
    }
    return isInRankBranch;
}

MPIrankCase MPIVisitor::collectMPICallsInCase(Stmt *then, Stmt *condition) {
    MPIrankCase rankCaseVector;
    StmtVisitor stmtVisitor{then};  // collect call exprs
    for (CallExpr *callExpr : stmtVisitor.callExprs_) {
        // filter mpi calls
        if (checker_.funcClassifier_.isMPIType(
                callExpr->getDirectCallee()->getIdentifier())) {
            // add to global vector
            MPICall::visitedCalls.emplace_back(callExpr, condition, false);
            // add reference to rankCase vector
            rankCaseVector.push_back(MPICall::visitedCalls.back());
        }
    }
    return rankCaseVector;
}

/**
 * Strips calls from two different rank cases where the mpi send and receive
 * functions match. This respects the order of functions inside the cases. While
 * nonblocking calls are just skipped in case of a mismatch, blocking calls on
 * the recv side that do not match quit the loop.
 *
 * @param rankCase1
 * @param rankCase2
 */
void MPIVisitor::stripPointToPointMatches(MPIrankCase &rankCase1,
                                          MPIrankCase &rankCase2) {
    size_t i2 = 0;
    for (size_t i = 0; i < rankCase2.size() && rankCase1.size(); ++i) {
        // skip non point to point
        if (!checker_.funcClassifier_.isPointToPointType(rankCase1[i2].get())) {
            i2++;

        } else if (checker_.isSendRecvPair(rankCase1[i2], rankCase2[i])) {
            // remove matched pair
            cont::eraseIndex(rankCase1, i2);
            cont::eraseIndex(rankCase2, i--);
        }
        // if non-matching, blocking function is hit, break
        else if (checker_.funcClassifier_.isBlockingType(rankCase2[i].get())) {
            break;
        }
    }
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

    // collect mpi calls in if
    rankCases.emplace_back(
        collectMPICallsInCase(ifStmt->getThen(), ifStmt->getCond()));

    // collect mpi calls in all else if
    Stmt *elseStmt = ifStmt->getElse();
    while (IfStmt *elseIf = dyn_cast_or_null<IfStmt>(elseStmt)) {
        rankCases.emplace_back(
            collectMPICallsInCase(elseIf->getThen(), elseIf->getCond()));
        elseStmt = elseIf->getElse();
    }

    // collect mpi calls in else
    if (elseStmt)
        rankCases.emplace_back(collectMPICallsInCase(elseStmt, nullptr));

    // save rank cases for complete translation unit analysis
    cont::copy(rankCases, MPIRankCases::visitedRankCases);

    // check if collective calls are used in rank rankCase
    for (const MPIrankCase &rankCase : rankCases) {
        for (const MPICall &call : rankCase) {
            checker_.checkForCollectiveCall(call);
        }
    }

    for (size_t i = 0; i < 2; ++i) {
        for (MPIrankCase &rankCase1 : rankCases) {
            for (MPIrankCase &rankCase2 : rankCases) {
                // compare by pointer
                if (&rankCase1 == &rankCase2) continue;
                stripPointToPointMatches(rankCase1, rankCase2);
            }
        }
    }

    checker_.checkUnmatchedCalls(rankCases);
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
        visitor.checker_.checkForRedundantCalls();

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
