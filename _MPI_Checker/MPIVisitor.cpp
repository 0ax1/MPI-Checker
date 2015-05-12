#ifndef MPIVISITOR_CPP_H6L3JFDT
#define MPIVISITOR_CPP_H6L3JFDT

#include <functional>
#include "MPIVisitor.hpp"
#include "llvm/ADT/SmallVector.h"

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
        checker_.bugReporter_.currentFunctionDecl_ = functionDecl;
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
    Stmt *then, Stmt *condition,
    llvm::SmallVector<Stmt *, 4> unmatchedConditions) {
    MPIrankCase rankCaseVector;
    StmtVisitor stmtVisitor{then};  // collect call exprs
    for (CallExpr *callExpr : stmtVisitor.callExprs_) {
        // filter mpi calls
        if (checker_.funcClassifier_.isMPIType(
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
 * Checker host class. Registers checker functionality.
 * Class name determines checker name to specify when the command line
 * is invoked for static analysis.
 * Receives callback for every translation unit about to visit.
 */
class MPIChecker
    : public Checker<check::ASTDecl<TranslationUnitDecl>,
                     check::PreStmt<CallExpr>, check::EndFunction> {
public:
    MPIChecker() { initBugTypes(); }

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

    // path sensitive callbacks––––––––––––––––––––––––––––––––––––––––––––
    void checkPreStmt(const CallExpr *callExpr, CheckerContext &ctx) const {
        setupFunctionClassifier(ctx);

        if (funcClassifier_->isWaitType(
                callExpr->getDirectCallee()->getIdentifier())) {
            checkWait(callExpr, ctx);
        } else if (funcClassifier_->isNonBlockingType(
                       callExpr->getDirectCallee()->getIdentifier())) {
            checkNonBlocking(callExpr, ctx);
        }
    }
    void checkNonBlocking(const CallExpr *callExpr, CheckerContext &ctx) const {
        ProgramStateRef state = ctx.getState();
        auto rankVars = state->get<RankVarMap>();

        MPICall mpiCall{const_cast<CallExpr *>(callExpr)};
        auto arg = mpiCall.arguments_[mpiCall.callExpr_->getNumArgs() - 1];
        auto requestVar = arg.vars_.front();
        const RankVar *rankVar = state->get<RankVarMap>(requestVar);
        state = state->set<RankVarMap>(
            requestVar, {requestVar, const_cast<CallExpr *>(callExpr)});
        auto node = ctx.addTransition(state);

        if (rankVar && rankVar->lastUser_) {
            auto lastUserID =
                rankVar->lastUser_->getDirectCallee()->getIdentifier();
            if (funcClassifier_->isNonBlockingType(lastUserID)) {
                std::string errorText{
                    "Request " + requestVar->getNameAsString() +
                    " is already in use by nonblocking call. "};

                BugReport *bugReport =
                    new BugReport(*DoubleImmediateBugType, errorText, node);
                bugReport->addRange(callExpr->getSourceRange());
                bugReport->addRange(requestVar->getSourceRange());
                ctx.emitReport(bugReport);
            }
        }
    }

    void checkWait(const CallExpr *callExpr, CheckerContext &ctx) const {
        ProgramStateRef state = ctx.getState();
        auto rankVars = state->get<RankVarMap>();

        // collect request vars
        MPICall mpiCall{const_cast<CallExpr *>(callExpr)};
        llvm::SmallVector<VarDecl *, 1> requestVector;
        if (funcClassifier_->isMPI_Wait(mpiCall)) {
            requestVector.push_back(mpiCall.arguments_[0].vars_.front());
        } else if (funcClassifier_->isMPI_Waitall(mpiCall)) {
            ArrayVisitor arrayVisitor{mpiCall.arguments_[1].vars_.front()};
            arrayVisitor.vars_.resize(arrayVisitor.vars_.size() / 2);  // hack

            for (auto &requestVar : arrayVisitor.vars_) {
                requestVector.push_back(requestVar);
            }
        }

        for (VarDecl *requestVar : requestVector) {
            const RankVar *rankVar = state->get<RankVarMap>(requestVar);
            state = state->set<RankVarMap>(
                requestVar, {requestVar, const_cast<CallExpr *>(callExpr)});
            auto node = ctx.addTransition(state);

            if (rankVar && rankVar->lastUser_) {
                auto lastUserID =
                    rankVar->lastUser_->getDirectCallee()->getIdentifier();
                // check for double wait
                if (funcClassifier_->isWaitType(lastUserID)) {
                    std::string errorText{"Request " +
                                          requestVar->getNameAsString() +
                                          " is already waited upon. "};

                    BugReport *bugReport =
                        new BugReport(*DoubleWaitBugType, errorText, node);
                    bugReport->addRange(callExpr->getSourceRange());
                    bugReport->addRange(requestVar->getSourceRange());
                    ctx.emitReport(bugReport);
                }
            }
            // no matching nonblocking call
            else {
                std::string errorText{"Request " +
                                      requestVar->getNameAsString() +
                                      " has no matching nonblocking call. "};

                BugReport *bugReport =
                    new BugReport(*UnmatchedWaitBugType, errorText, node);
                bugReport->addRange(callExpr->getSourceRange());
                bugReport->addRange(requestVar->getSourceRange());
                ctx.emitReport(bugReport);
            }
        }
    }

    void checkEndFunction(CheckerContext &ctx) const {
        ProgramStateRef state = ctx.getState();
        auto rankVars = state->get<RankVarMap>();
        auto node = ctx.addTransition();

        setupFunctionClassifier(ctx);

        // prüfe  ob letzte funkion eine sendende und keine wait ist
        // kein match für immediate
        MPIBugReporter bugReporter{ctx.getBugReporter(), *this,
                                   ctx.getAnalysisManager()};
        // at the end of a function immediate calls should be matched with wait
        for (auto &rankVar : rankVars) {
            if (rankVar.second.lastUser_ &&
                funcClassifier_->isNonBlockingType(
                    rankVar.second.lastUser_->getDirectCallee()
                        ->getIdentifier())) {
                std::string errorText{
                    "Nonblocking call using request " +
                    rankVar.second.varDecl_->getNameAsString() +
                    " has no matching wait. "};

                BugReport *bugReport =
                    new BugReport(*UnmatchedImmediateBugType, errorText, node);
                bugReport->addRange(rankVar.second.lastUser_->getSourceRange());
                bugReport->addRange(rankVar.second.varDecl_->getSourceRange());
                ctx.emitReport(bugReport);
            }
        }

        // remove all rank vars
        for (auto &rankVar : rankVars) {
            state = state->remove<RankVarMap>(rankVar.first);
        }
        ctx.addTransition(state);
    }

private:
    void initBugTypes() {
        DoubleWaitBugType.reset(new BugType(this, "double wait", "MPI Error"));
        UnmatchedWaitBugType.reset(
            new BugType(this, "unmatched wait", "MPI Error"));
        DoubleImmediateBugType.reset(
            new BugType(this, "double request usage", "MPI Error"));
        UnmatchedImmediateBugType.reset(
            new BugType(this, "missing wait", "MPI Error"));
    }

    void setupFunctionClassifier(CheckerContext &ctx) const {
        if (!funcClassifier_) {
            funcClassifier_.reset(
                new MPIFunctionClassifier(ctx.getAnalysisManager()));
        }
    }
    std::unique_ptr<BugType> UnmatchedWaitBugType;
    std::unique_ptr<BugType> UnmatchedImmediateBugType;
    std::unique_ptr<BugType> DoubleWaitBugType;
    std::unique_ptr<BugType> DoubleImmediateBugType;
    mutable std::unique_ptr<MPIFunctionClassifier> funcClassifier_{nullptr};
};
}  // end of namespace: mpi

void ento::registerMPIChecker(CheckerManager &mgr) {
    mgr.registerChecker<mpi::MPIChecker>();
}

#endif  // end of include guard: MPIVISITOR_CPP_H6L3JFDT
