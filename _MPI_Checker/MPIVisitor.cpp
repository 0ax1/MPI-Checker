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

std::vector<std::reference_wrapper<MPICall>>
MPIVisitor::collectMPICallsInCase(Stmt *then, Stmt *condition) {
    std::vector<std::reference_wrapper<MPICall>> ifCaseVector;
    StmtVisitor stmtVisitor{then};  // collect call exprs
    for (CallExpr *callExpr : stmtVisitor.callExprs_) {
        // filter mpi calls
        if (checker_.funcClassifier_.isMPIType(
                callExpr->getDirectCallee()->getIdentifier())) {
            // add to global vector
            MPICall::visitedCalls.emplace_back(callExpr, condition, false);
            // add reference to ifCase vector
            ifCaseVector.push_back(MPICall::visitedCalls.back());
        }
    }
    return ifCaseVector;
}

/**
 * Visits ifCases. Checks if a rank variable is involved.
 *
 * @param ifStmt
 *
 * @return
 */
bool MPIVisitor::VisitIfStmt(IfStmt *ifStmt) {
    // vector element must be assignable -> use std:vector is inside
    llvm::SmallVector<std::vector<std::reference_wrapper<MPICall>>, 4> ifCases;

    // only inspect rank ifCases
    if (!isRankBranch(ifStmt)) return true;

    // collect mpi calls in if
    ifCases.emplace_back(
        collectMPICallsInCase(ifStmt->getThen(), ifStmt->getCond()));

    // collect mpi calls in all else if
    Stmt *elseStmt = ifStmt->getElse();
    while (IfStmt *elseIf = dyn_cast_or_null<IfStmt>(elseStmt)) {
        ifCases.emplace_back(
            collectMPICallsInCase(elseIf->getThen(), elseIf->getCond()));
        elseStmt = elseIf->getElse();
    }

    // collect mpi calls in else
    if (elseStmt)
        ifCases.emplace_back(collectMPICallsInCase(elseStmt, nullptr));

    // check if collective calls are used in rank ifCase
    for (auto &ifCase : ifCases) {
        for (const MPICall &call : ifCase) {
            checker_.checkForCollectiveInCase(call);
        }
    }

    for (size_t i = 0; i < 2; ++i) {
        for (auto &ifCase1 : ifCases) {
            for (auto &ifCase2 : ifCases) {
                // compare by pointer of vector
                if (&ifCase1 == &ifCase2) continue;
                size_t i2 = 0;
                for (size_t i = 0; i < ifCase2.size() && ifCase1.size(); ++i) {
                    // skip non point to point
                    if (!checker_.funcClassifier_.isPointToPointType(
                            ifCase1[i2].get().identInfo_)) {
                        i2++;

                    } else if (checker_.isSendRecvPair(ifCase1[i2],
                                                       ifCase2[i])) {
                        // remove matched calls
                        cont::eraseIndex(ifCase1, i2);
                        cont::eraseIndex(ifCase2, i--);
                    } else if (checker_.funcClassifier_.isBlockingType(
                                   ifCase2[i].get().identInfo_)) {
                        break;
                    }
                }
            }
        }
    }

    for (auto &ifCase : ifCases) {
        for (const MPICall &b : ifCase) {
            if (checker_.funcClassifier_.isPointToPointType(b.identInfo_)) {
                llvm::outs() << "unmatched " + b.identInfo_->getName() << "\n";
            }
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

        if (checker_.funcClassifier_.isCollectiveType(mpiCall.identInfo_)) {
            MPICall::visitedCalls.push_back(std::move(mpiCall));
        }
        checker_.checkRequestUsage(mpiCall);
    }

    return true;
}

void MPIVisitor::trackRankVariables(const MPICall &mpiCall) const {
    if (checker_.funcClassifier_.isMPI_Comm_rank(mpiCall.identInfo_)) {
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

        // clear visited calls after every translation unit
        MPICall::visitedCalls.clear();
        MPIRequest::MPIRequest::visitedRequests.clear();
    }
};

}  // end of namespace: mpi

void ento::registerMPIChecker(CheckerManager &mgr) {
    mgr.registerChecker<mpi::MPIChecker>();
}

#endif  // end of include guard: MPIVISITOR_CPP_H6L3JFDT
