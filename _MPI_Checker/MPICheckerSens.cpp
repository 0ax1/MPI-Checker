#include "MPICheckerSens.hpp"

namespace mpi {

using namespace clang;
using namespace ento;

void MPICheckerSens::checkDoubleNonblocking(const CallExpr *callExpr,
                                           CheckerContext &ctx) const {
    if (!funcClassifier_.isNonBlockingType(
            callExpr->getDirectCallee()->getIdentifier())) {
        return;
    }

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
        if (funcClassifier_.isNonBlockingType(lastUserID)) {
            bugReporter_.reportDoubleNonblocking(requestVar, callExpr, node);
        }
    }
}

void MPICheckerSens::checkWaitUsage(const CallExpr *callExpr,
                                    CheckerContext &ctx) const {
    if (!funcClassifier_.isWaitType(
            callExpr->getDirectCallee()->getIdentifier())) {
        return;
    }

    ProgramStateRef state = ctx.getState();
    auto rankVars = state->get<RankVarMap>();

    // collect request vars
    MPICall mpiCall{const_cast<CallExpr *>(callExpr)};
    llvm::SmallVector<VarDecl *, 1> requestVector;
    if (funcClassifier_.isMPI_Wait(mpiCall)) {
        requestVector.push_back(mpiCall.arguments_[0].vars_.front());
    } else if (funcClassifier_.isMPI_Waitall(mpiCall)) {
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
        auto node = ctx.addTransition();

        if (rankVar && rankVar->lastUser_) {
            auto lastUserID =
                rankVar->lastUser_->getDirectCallee()->getIdentifier();
            // check for double wait
            if (funcClassifier_.isWaitType(lastUserID)) {
                bugReporter_.reportDoubleWait(callExpr, *rankVar, node);
            }
        }
        // no matching nonblocking call
        else {
            bugReporter_.reportUnmatchedWait(callExpr, requestVar, node);
        }

        ctx.addTransition(state);
    }
}

void MPICheckerSens::checkMissingWait(CheckerContext &ctx) {
    ProgramStateRef state = ctx.getState();
    auto rankVars = state->get<RankVarMap>();
    ExplodedNode *node = ctx.addTransition();
    // at the end of a function immediate calls should be matched with wait
    for (auto &rankVar : rankVars) {
        if (rankVar.second.lastUser_ &&
            funcClassifier_.isNonBlockingType(
                rankVar.second.lastUser_->getDirectCallee()->getIdentifier())) {
            bugReporter_.reportMissingWait(rankVar.second, node);
        }
    }
}


void MPICheckerSens::clearRankVars(CheckerContext &ctx) const {
    ProgramStateRef state = ctx.getState();
    auto rankVars = state->get<RankVarMap>();
    // clear rank container
    for (auto &rankVar : rankVars) {
        state = state->remove<RankVarMap>(rankVar.first);
    }
    ctx.addTransition(state);
}

}  // end of namespace: mpi
