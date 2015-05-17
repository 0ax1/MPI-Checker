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
    auto RequestVars = state->get<RequestVarMap>();

    MPICall mpiCall{const_cast<CallExpr *>(callExpr)};
    auto arg = mpiCall.arguments_[mpiCall.callExpr_->getNumArgs() - 1];
    auto requestVarDecl = arg.vars_.front();
    const RequestVar *requestVar = state->get<RequestVarMap>(requestVarDecl);
    state = state->set<RequestVarMap>(
        requestVarDecl, {requestVarDecl, const_cast<CallExpr *>(callExpr)});
    auto node = ctx.addTransition(state);

    if (requestVar && requestVar->lastUser_) {
        auto lastUserID =
            requestVar->lastUser_->getDirectCallee()->getIdentifier();
        if (funcClassifier_.isNonBlockingType(lastUserID)) {
            bugReporter_.reportDoubleNonblocking(callExpr, *requestVar, node);
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
    auto requestVars = state->get<RequestVarMap>();

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

    auto node = ctx.addTransition();

    for (VarDecl *requestVarDecl : requestVector) {
        const RequestVar *requestVar =
            state->get<RequestVarMap>(requestVarDecl);
        state = state->set<RequestVarMap>(
            requestVarDecl, {requestVarDecl, const_cast<CallExpr *>(callExpr)});

        if (requestVar && requestVar->lastUser_) {
            auto lastUserID =
                requestVar->lastUser_->getDirectCallee()->getIdentifier();
            // check for double wait
            if (funcClassifier_.isWaitType(lastUserID)) {
                bugReporter_.reportDoubleWait(callExpr, *requestVar, node);
            }
        }
        // no matching nonblocking call
        else {
            bugReporter_.reportUnmatchedWait(callExpr, requestVarDecl, node);
        }
    }

    ctx.addTransition(state);
}

void MPICheckerSens::checkMissingWait(CheckerContext &ctx) {
    ProgramStateRef state = ctx.getState();
    auto requestVars = state->get<RequestVarMap>();
    ExplodedNode *node = ctx.addTransition();
    // at the end of a function immediate calls should be matched with wait
    for (auto &requestVar : requestVars) {
        if (requestVar.second.lastUser_ &&
            funcClassifier_.isNonBlockingType(
                requestVar.second.lastUser_->getDirectCallee()
                    ->getIdentifier())) {
            bugReporter_.reportMissingWait(requestVar.second, node);
        }
    }
}

void MPICheckerSens::clearRequestVars(CheckerContext &ctx) const {
    ProgramStateRef state = ctx.getState();
    auto requestVars = state->get<RequestVarMap>();
    // clear rank container
    for (auto &requestVar : requestVars) {
        state = state->remove<RequestVarMap>(requestVar.first);
    }
    ctx.addTransition(state);
}

}  // end of namespace: mpi