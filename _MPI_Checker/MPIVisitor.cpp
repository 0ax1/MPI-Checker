#include <functional>
#include "MPIVisitor.hpp"
#include "llvm/ADT/SmallVector.h"
#include "MPICheckerSens.hpp"

#include "ClangSACheckers.h"
#include "InterCheckerAPI.h"
#include "clang/AST/Attr.h"
#include "clang/AST/ParentMap.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugType.h"
#include "clang/StaticAnalyzer/Core/Checker.h"
#include "clang/StaticAnalyzer/Core/CheckerManager.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CallEvent.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/ProgramState.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/ProgramStateTrait.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/SymbolManager.h"
#include "llvm/ADT/ImmutableMap.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringExtras.h"

using namespace clang;
using namespace ento;

namespace mpi {

// visitor functions –––––––––––––––––––––––––––––––––––––––––––––––––––––
bool MPIVisitor::VisitFunctionDecl(FunctionDecl *functionDecl) {
    // to keep track which function implementation is currently analysed
    if (functionDecl->clang::Decl::hasBody() && !functionDecl->isInlined()) {
        // to make display of function in diagnostics available
        checkerAST_.bugReporter_.currentFunctionDecl_ = functionDecl;
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
 * Visits all if and else if!
 *
 * @param ifStmt
 *
 * @return
 */
bool MPIVisitor::VisitIfStmt(IfStmt *ifStmt) {
    if (!isRankBranch(ifStmt)) return true;  // only inspect rank branches
    if (cont::isContained(visitedIfStmts_, ifStmt)) return true;

    llvm::SmallVector<Stmt *, 4> unmatchedConditions;

    // collect mpi calls in if / else if
    Stmt *stmt = ifStmt;
    while (IfStmt *ifStmt = dyn_cast_or_null<IfStmt>(stmt)) {
        MPIRankCases::visitedRankCases.emplace_back(
            collectMPICallsInCase(ifStmt->getThen(), ifStmt->getCond(), {}));
        unmatchedConditions.push_back(ifStmt->getCond());
        stmt = ifStmt->getElse();
        visitedIfStmts_.push_back(ifStmt);
    }

    // collect mpi calls in else
    if (stmt) {
        MPIRankCases::visitedRankCases.emplace_back(
            collectMPICallsInCase(stmt, nullptr, unmatchedConditions));
    }

    // check if collective calls are used in rank rankCase
    for (const MPIrankCase &rankCase : MPIRankCases::visitedRankCases) {
        for (const MPICall &call : rankCase) {
            checkerAST_.checkForCollectiveCall(call);
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

    if (checkerAST_.funcClassifier_.isMPIType(functionDecl->getIdentifier())) {
        MPICall mpiCall{callExpr};

        checkerAST_.checkBufferTypeMatch(mpiCall);
        checkerAST_.checkForInvalidArgs(mpiCall);
        trackRankVariables(mpiCall);

        if (checkerAST_.funcClassifier_.isCollectiveType(mpiCall)) {
            MPICall::visitedCalls.push_back(std::move(mpiCall));
        }
    }

    return true;
}

void MPIVisitor::trackRankVariables(const MPICall &mpiCall) const {
    if (checkerAST_.funcClassifier_.isMPI_Comm_rank(mpiCall)) {
        VarDecl *varDecl = mpiCall.arguments_[1].vars_[0];
        MPIRank::visitedRankVariables.insert(varDecl);
    }
}

MPIrankCase MPIVisitor::collectMPICallsInCase(
    Stmt *then, Stmt *condition,
    llvm::SmallVector<Stmt *, 4> unmatchedConditions) {
    MPIrankCase rankCaseVector;
    StmtVisitor stmtVisitor{then};  // collect call exprs
    for (CallExpr *callExpr : stmtVisitor.callExprs_) {
        // filter mpi calls
        if (checkerAST_.funcClassifier_.isMPIType(
                callExpr->getDirectCallee()->getIdentifier())) {
            if (unmatchedConditions.size()) {
                MPICall::visitedCalls.emplace_back(callExpr, condition,
                                                   unmatchedConditions);
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
 * Checker host class that registers the checker for static analysis.
 * Class name determines checker name to specify on the command line.
 * Is created once for every translation unit.
 */
class MPIChecker
    : public Checker<check::ASTDecl<TranslationUnitDecl>,
                     check::PreStmt<CallExpr>, check::EndFunction> {
public:
    // ast callback–––––––––––––––––––––––––––––––––––––––––––––––––––––––
    void checkASTDecl(const TranslationUnitDecl *tuDecl,
                      AnalysisManager &analysisManager,
                      BugReporter &bugReporter) const {
        MPIVisitor visitor{bugReporter, *this, analysisManager};
        visitor.TraverseTranslationUnitDecl(
            const_cast<TranslationUnitDecl *>(tuDecl));

        // invoked after travering the translation unit
        // visitor.checkerAST_.checkForRedundantCalls();
        visitor.checkerAST_.checkPointToPointSchema();

        // clear after every translation unit
        MPICall::visitedCalls.clear();
        MPIRequest::visitedRequests.clear();
        MPIRankCases::visitedRankCases.clear();
    }

    // path sensitive callbacks––––––––––––––––––––––––––––––––––––––––––––
    void checkPreStmt(const CallExpr *callExpr, CheckerContext &ctx) const {
        ctx.getBugReporter();
        dynamicInit(ctx);
        checkerSens_->checkWaitUsage(callExpr, ctx);
        checkerSens_->checkDoubleNonblocking(callExpr, ctx);
    }

    void checkEndFunction(CheckerContext &ctx) const {
        dynamicInit(ctx);
        checkerSens_->checkMissingWait(ctx);
        checkerSens_->clearRankVars(ctx);
    }

private:
    const std::unique_ptr<MPICheckerSens> checkerSens_;

    void dynamicInit(CheckerContext &ctx) const {
        if (!checkerSens_) {
            const_cast<std::unique_ptr<MPICheckerSens> &>(checkerSens_)
                .reset(new MPICheckerSens(ctx.getAnalysisManager(), this,
                                          ctx.getBugReporter()));
        }
    }
};
}  // end of namespace: mpi

void ento::registerMPIChecker(CheckerManager &mgr) {
    mgr.registerChecker<mpi::MPIChecker>();
}
