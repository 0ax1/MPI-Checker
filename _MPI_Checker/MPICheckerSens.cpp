#include "MPICheckerSens.hpp"

namespace mpi {

using namespace clang;
using namespace ento;

void MPICheckerSens::initBugTypes() {
    DoubleWaitBugType.reset(
        new BugType(checkerBase_, "double wait", "MPI Error"));
    UnmatchedWaitBugType.reset(
        new BugType(checkerBase_, "unmatched wait", "MPI Error"));
    DoubleRequestBugType.reset(
        new BugType(checkerBase_, "double request usage", "MPI Error"));
    MissingWaitBugType.reset(
        new BugType(checkerBase_, "missing wait", "MPI Error"));
}

void MPICheckerSens::checkNonBlockingUsage(const CallExpr *callExpr,
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
            std::string errorText{"Request " + requestVar->getNameAsString() +
                                  " is already in use by nonblocking call. "};

            BugReport *bugReport =
                new BugReport(*DoubleRequestBugType, errorText, node);
            bugReport->addRange(callExpr->getSourceRange());
            bugReport->addRange(requestVar->getSourceRange());
            ctx.emitReport(bugReport);
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
        auto node = ctx.addTransition(state);

        if (rankVar && rankVar->lastUser_) {
            auto lastUserID =
                rankVar->lastUser_->getDirectCallee()->getIdentifier();
            // check for double wait
            if (funcClassifier_.isWaitType(lastUserID)) {
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
            std::string errorText{"Request " + requestVar->getNameAsString() +
                                  " has no matching nonblocking call. "};

            BugReport *bugReport =
                new BugReport(*UnmatchedWaitBugType, errorText, node);
            bugReport->addRange(callExpr->getSourceRange());
            bugReport->addRange(requestVar->getSourceRange());
            ctx.emitReport(bugReport);
        }
    }
}

void MPICheckerSens::checkForUnmatchedWait(CheckerContext &ctx) {
    ProgramStateRef state = ctx.getState();
    auto rankVars = state->get<RankVarMap>();
    auto node = ctx.addTransition();
    // at the end of a function immediate calls should be matched with wait
    for (auto &rankVar : rankVars) {
        if (rankVar.second.lastUser_ &&
            funcClassifier_.isNonBlockingType(
                rankVar.second.lastUser_->getDirectCallee()->getIdentifier())) {
            std::string errorText{"Nonblocking call using request " +
                                  rankVar.second.varDecl_->getNameAsString() +
                                  " has no matching wait. "};

            BugReport *bugReport =
                new BugReport(*MissingWaitBugType, errorText, node);
            bugReport->addRange(rankVar.second.lastUser_->getSourceRange());
            bugReport->addRange(rankVar.second.varDecl_->getSourceRange());
            ctx.emitReport(bugReport);
        }
    }
}

}  // end of namespace: mpi
