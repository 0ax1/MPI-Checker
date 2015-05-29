#include "MPICheckerPathSensitive.hpp"
#include "ArrayVisitor.hpp"

namespace mpi {

using namespace clang;
using namespace ento;

/**
 * Checks if a request is used by nonblocking calls multiple times
 * before intermediate wait.
 *
 * @param callExpr
 * @param ctx
 */
void MPICheckerPathSensitive::checkDoubleNonblocking(
    const CallExpr *callExpr, CheckerContext &ctx) const {
    if (!funcClassifier_.isNonBlockingType(
            callExpr->getDirectCallee()->getIdentifier())) {
        return;
    }

    ProgramStateRef state = ctx.getState();
    auto RequestVars = state->get<RequestVarMap>();

    MPICall mpiCall{const_cast<CallExpr *>(callExpr)};
    auto arg = mpiCall.arguments()[mpiCall.callExpr()->getNumArgs() - 1];
    auto requestVarDecl = arg.vars().front();
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

/**
 * Checks if a request is used by wait multiple times without intermediate
 * nonblocking call.
 *
 * @param callExpr
 * @param ctx
 */
void MPICheckerPathSensitive::checkWaitUsage(const CallExpr *callExpr,
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
        requestVector.push_back(mpiCall.arguments()[0].vars().front());
    } else if (funcClassifier_.isMPI_Waitall(mpiCall)) {
        ArrayVisitor arrayVisitor{mpiCall.arguments()[1].vars().front()};

        for (const auto &requestVar : arrayVisitor.vars()) {
            requestVector.push_back(requestVar);
        }
    }

    const ExplodedNode *const node = ctx.addTransition();

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

/**
 * Check if a nonblocking call has no matching wait.
 *
 * @param ctx
 */
void MPICheckerPathSensitive::checkMissingWaits(CheckerContext &ctx) {
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

/**
 * Erase all request vars from the path sensitive map.
 *
 * @param ctx
 */
void MPICheckerPathSensitive::clearRequestVars(CheckerContext &ctx) const {
    ProgramStateRef state = ctx.getState();
    auto requestVars = state->get<RequestVarMap>();
    // clear rank container
    for (auto &requestVar : requestVars) {
        state = state->remove<RequestVarMap>(requestVar.first);
    }
    ctx.addTransition(state);
}

}  // end of namespace: mpi
