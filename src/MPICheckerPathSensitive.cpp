/*
 The MIT License (MIT)

 Copyright (c) 2015 Alexander Droste

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
*/

#include "MPICheckerPathSensitive.hpp"
#include "Utility.hpp"

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
    const clang::ento::CallEvent &callEvent, CheckerContext &ctx) const {
    if (!funcClassifier_.isNonBlockingType(callEvent.getCalleeIdentifier())) {
        return;
    }

    ProgramStateRef state = ctx.getState();
    CallEventRef<> callEventRef = callEvent.cloneWithState(state);
    const MemRegion *memRegion =
        callEvent.getArgSVal(callEvent.getNumArgs() - 1).getAsRegion();

    const RequestVar *requestVar = state->get<RequestVarMap>(memRegion);
    const ExplodedNode *const node = ctx.addTransition(nullptr);

    if (requestVar) {
        if (funcClassifier_.isNonBlockingType(
                requestVar->lastUser_->getCalleeIdentifier())) {
            bugReporter_.reportDoubleNonblocking(callEvent, *requestVar, node);
        }
    }

    state = state->set<RequestVarMap>(memRegion,
                                      mpi::RequestVar{memRegion, callEventRef});
    ctx.addTransition(state);
}

const MemRegion *MPICheckerPathSensitive::requestMemRegion(
    const clang::ento::CallEvent &callEvent) const {
    if (funcClassifier_.isMPI_Wait(callEvent.getCalleeIdentifier())) {
        return callEvent.getArgSVal(0).getAsRegion();
    } else if (funcClassifier_.isMPI_Waitall(callEvent.getCalleeIdentifier())) {
        return callEvent.getArgSVal(1).getAsRegion();
    } else {
        return (const MemRegion *)nullptr;
    }
}

/**
 * Checks if a request is used by wait multiple times without intermediate
 * nonblocking call.
 *
 * @param callExpr
 * @param ctx
 */
void MPICheckerPathSensitive::checkWaitUsage(
    const clang::ento::CallEvent &callEvent, CheckerContext &ctx) const {
    if (!funcClassifier_.isWaitType(callEvent.getCalleeIdentifier())) return;

    const MemRegion *memRegion = requestMemRegion(callEvent);
    if (!memRegion) return;

    ProgramStateRef state = ctx.getState();
    CallEventRef<> callEventRef = callEvent.cloneWithState(state);
    const ExplodedNode *const node = ctx.addTransition();

    // requests are associcated with a VarRegion
    const VarRegion *varRegion =
        dyn_cast<VarRegion>(memRegion->getBaseRegion());
    MemRegionManager *regionManager = varRegion->getMemRegionManager();
    llvm::SmallVector<const MemRegion *, 2> requestRegions;

    if (funcClassifier_.isMPI_Waitall(callEvent.getCalleeIdentifier())) {
        auto size = ctx.getStoreManager().getSizeInElements(
            state, memRegion->getBaseRegion(),
            callEvent.getArgExpr(1)->getType()->getPointeeType());

        const llvm::APSInt &arrSize =
            size.getAs<nonloc::ConcreteInt>()->getValue();

        for (size_t i = 0; i < arrSize; ++i) {
            NonLoc idx = ctx.getSValBuilder().makeArrayIndex(i);

            const ElementRegion *elementRegion =
                regionManager->getElementRegion(
                    callEvent.getArgExpr(1)->getType()->getPointeeType(), idx,
                    varRegion, ctx.getASTContext());

            requestRegions.push_back(elementRegion->getAs<MemRegion>());
        }
    } else if (funcClassifier_.isMPI_Wait(callEvent.getCalleeIdentifier())) {
        requestRegions.push_back(memRegion);
    }

    // check all requestRegions used in wait function
    for (const auto requestRegion : requestRegions) {
        const RequestVar *requestVar = state->get<RequestVarMap>(requestRegion);
        state = state->set<RequestVarMap>(requestRegion,
                                          {requestRegion, callEventRef});

        if (requestVar) {
            // check for double wait
            if (funcClassifier_.isWaitType(
                    requestVar->lastUser_->getCalleeIdentifier())) {
                bugReporter_.reportDoubleWait(callEvent, *requestVar, node);
            }
        }
        // no matching nonblocking call
        else {
            bugReporter_.reportUnmatchedWait(callEvent, requestRegion, node);
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
                requestVar.second.lastUser_->getCalleeIdentifier())) {
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
