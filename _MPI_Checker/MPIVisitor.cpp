#ifndef MPIVISITOR_CPP_H6L3JFDT
#define MPIVISITOR_CPP_H6L3JFDT

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

/**
 * Visits branches. Checks if a rank variable is involved.
 *
 * @param ifStmt
 *
 * @return
 */
bool MPIVisitor::VisitIfStmt(IfStmt *ifStmt) {
    ExprVisitor exprVisitor{ifStmt->getCond()};

    bool isRankBranch{false};
    for (const VarDecl *const varDecl : exprVisitor.vars_) {
        if (cont::isContained(MPIRank::visitedRankVariables, varDecl)) {
            isRankBranch = true;
            break;
        }
    }

    llvm::SmallVector<MPICall, 16> visitedCallsBranch;
    auto collectExpr =
        [this](llvm::SmallVector<MPICall, 16> &v, Stmt *then, Stmt *condition) {
            StmtVisitor stmtVisitor{then};  // collect call exprs
            for (CallExpr *callExpr : stmtVisitor.callExprs_) {
                // filter mpi calls
                if (checker_.funcClassifier_.isMPIType(
                        callExpr->getDirectCallee()->getIdentifier())) {
                    MPICall mpiCall{callExpr, condition, false};
                    v.push_back(mpiCall);
                }
            }
        };

    if (isRankBranch) {
        // collect mpi calls in if
        collectExpr(visitedCallsBranch, ifStmt->getThen(), ifStmt->getCond());

        // collect mpi calls in all else if
        Stmt *elseStmt = ifStmt->getElse();  // else(if)
        while (IfStmt *elseIf = dyn_cast_or_null<IfStmt>(elseStmt)) {
            collectExpr(visitedCallsBranch, elseIf->getThen(),
                        elseIf->getCond());
            elseStmt = elseIf->getElse();
        }

        // collect mpi calls in else
        if (elseStmt) {
            collectExpr(visitedCallsBranch, elseStmt, nullptr);
        }

        // check if collective calls are used in rank branch
        for (const MPICall &call : visitedCallsBranch) {
            checker_.checkCollCallInBranch(call);
        }

        // TODO evaluate calls
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
            checker_.checkRequestUsage(mpiCall);
        }
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
