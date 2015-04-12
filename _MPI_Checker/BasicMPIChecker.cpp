#include "ClangSACheckers.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugType.h"
#include "clang/StaticAnalyzer/Core/Checker.h"
#include "clang/StaticAnalyzer/Core/CheckerManager.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CallEvent.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/ProgramStateTrait.h"
#include "llvm/ADT/ImmutableMap.h"
#include <iostream>
#include <vector>

#include "container.hpp"
#include "Typedefs.hpp"

// checker bool: is optimistic?
// pointer escape (add state escaped?)
// call event is global function
// message state is nullptr if unknown
// -> why if is checking for nullptr first
// type of SVal can be asked

using namespace clang;
using namespace ento;


class Message {
private:
public:
    // states are const
    const enum class State { kSent, kRecvd } state_;
    const CallEventRef<> callEvent_;

    Message(const CallEventRef<> event, State initialState)
        : state_{initialState}, callEvent_{event} {}

    // passed type is message state
    // identity is defined with value of state_
    bool operator==(const Message &message) const {
        bool isEqual{true};
        if (callEvent_->getNumArgs() == message.callEvent_->getNumArgs()) {
            // check if all args are equal
            for (size_t i = 0; i < callEvent_->getNumArgs(); ++i) {
                if (callEvent_->getArgSVal(i) !=
                    message.callEvent_->getArgSVal(i)) {
                    isEqual = false;
                    break;
                }
            }
        } else {
            isEqual = false;
        }

        // check message state additionally
        if (state_ != message.state_) {
            isEqual = false;
        }

        return isEqual;
    }

    bool operator<(const Message &message) const {
        return callEvent_->getNumArgs() < message.callEvent_->getNumArgs();
    }

    // useful to check if nodes are in the same execution state
    void Profile(llvm::FoldingSetNodeID &foldingNodeId) const {
        // the data structure (here int) used to descrive the state
        foldingNodeId.AddBoolean(static_cast<bool>(state_));
        foldingNodeId.AddPointer(&callEvent_);
    }
};

// to capture custom state type in the cfg with a list
REGISTER_SET_WITH_PROGRAMSTATE(StateList, Message)

// class which messagechecker inherits from specifies checker type
class MessageChecker : public Checker<check::PostCall> {
    // II -> identifier info
    mutable IdentifierInfo *IdentInfo_MPI_Send, *IdentInfo_MPI_Recv,
        *IdentInfo_MPI_Isend, *IdentInfo_MPI_Irecv;

    // inform clang about new kind of bug types
    std::unique_ptr<BugType> DoubleRecvBugType;
    std::unique_ptr<BugType> DuplicateSendBugType;
    std::unique_ptr<BugType> UnmatchedRecvBugType;

    void initIdentifierInfo(ASTContext &Ctx) const;

    void reportDuplicateRecv(const CallEvent &, CheckerContext &) const;
    void reportDuplicateSend(const CallEvent &, CheckerContext &) const;
    void reportUnmatchedRecv(const CallEvent &, CheckerContext &) const;

    bool isIdenticalCallInList(const CallEvent &, StateListTy) const;
    bool areCommArgsConst(const CallEvent &) const;

    bool matchConstRecv(const CallEvent &, const CallEvent &) const;
    bool matchDynRecv(const CallEvent &, const CallEvent &) const;

    void checkForMatchingSend(const CallEvent &callEvent,
                              CheckerContext &context) const;

public:
    MessageChecker();
    void checkPostCall(const CallEvent &, CheckerContext &) const;
};

MessageChecker::MessageChecker()
    : IdentInfo_MPI_Send(0), IdentInfo_MPI_Recv(0) {
    // Initialize the bug types.
    // DoubleRecvBugType.reset(
    // new BugType(this, "unmatched receive", "MPI Error"));

    DuplicateSendBugType.reset(
        new BugType(this, "duplicate send", "MPI Error"));

    UnmatchedRecvBugType.reset(
        new BugType(this, "unmatched receive", "MPI Error"));
}

void MessageChecker::initIdentifierInfo(ASTContext &Ctx) const {
    // only init identifiers once
    if (IdentInfo_MPI_Send) return;

    // init function identifier
    IdentInfo_MPI_Send = &Ctx.Idents.get("MPI_Send");
    IdentInfo_MPI_Recv = &Ctx.Idents.get("MPI_Recv");
    IdentInfo_MPI_Isend = &Ctx.Idents.get("MPI_Isend");
    IdentInfo_MPI_Irecv = &Ctx.Idents.get("MPI_Irecv");
}

// ty = type
bool MessageChecker::isIdenticalCallInList(const CallEvent &callEvent,
                                           StateListTy list) const {
    // check for identical call
    for (const Message &mess : list) {
        // if calls have the same identifier -
        // implies they have the same number of args
        if (mess.callEvent_->getCalleeIdentifier() ==
            callEvent.getCalleeIdentifier()) {
            const size_t numArgs{callEvent.getNumArgs()};
            bool identical{true};
            for (size_t i = 0; i < numArgs; ++i) {
                if (callEvent.getArgSVal(i) != mess.callEvent_->getArgSVal(i)) {
                    identical = false;
                    break;
                }
            }
            // end if identical call was found
            if (identical) {
                return true;
            }
        }
    }
    return false;
}

/**
 * Checks if all arguments used in communication are const.
 *
 * @param callEvent function call to check the args for
 *
 * @return all args are const
 */
bool MessageChecker::areCommArgsConst(const CallEvent &callEvent) const {
    bool areConstant{true};
    std::vector<unsigned char> indices;

    // data-buf, mpi-macros, mpi-status, mpi-communicator
    // are skipped when checking for constness
    if (callEvent.getCalleeIdentifier() == IdentInfo_MPI_Send) {
        indices = MPI_Send_varCommArgs;
    } else if (callEvent.getCalleeIdentifier() == IdentInfo_MPI_Recv) {
        indices = MPI_Recv_varCommArgs;
    } else if (callEvent.getCalleeIdentifier() == IdentInfo_MPI_Isend) {
        indices = MPI_Isend_varCommArgs;
    } else if (callEvent.getCalleeIdentifier() == IdentInfo_MPI_Irecv) {
        indices = MPI_Irecv_varCommArgs;
    }

    for (unsigned char idx : indices) {
        if (!callEvent.getArgSVal(idx).isConstant()) {
            areConstant = false;
            break;
        }
    }

    return areConstant;
}

// TODO maps mit argumenten für funktionen definieren?
// key destination, value index des arguments
// enum mit mpi argument als schlüssel, source, tag, dest, ...

bool MessageChecker::matchConstRecv(const CallEvent &send,
                                    const CallEvent &recv) const {
    bool matchSuccesful{false};

    if (send.getCalleeIdentifier() == IdentInfo_MPI_Send &&
        recv.getCalleeIdentifier() == IdentInfo_MPI_Recv) {
        matchSuccesful = send.getArgSVal(MPI_Send_idx::kCount) ==
                             recv.getArgSVal(MPI_Recv_idx::kCount) &&

                         send.getArgSVal(MPI_Send_idx::kDatatype) ==
                             recv.getArgSVal(MPI_Recv_idx::kDatatype) &&

                         send.getArgSVal(MPI_Send_idx::kDest) ==
                             recv.getArgSVal(MPI_Recv_idx::kSource) &&

                         send.getArgSVal(MPI_Send_idx::kTag) ==
                             recv.getArgSVal(MPI_Recv_idx::kTag) &&

                         send.getArgSVal(MPI_Send_idx::kComm) ==
                             recv.getArgSVal(MPI_Send_idx::kComm);

    } else {
        // assert this part of code should not be reached
        llvm_unreachable("no send/recv pair to check found");
    }

    return matchSuccesful;
}

bool MessageChecker::matchDynRecv(const CallEvent &send,
                                  const CallEvent &recv) const {
    // TODO implement dyn matching
    bool matchSuccesful{false};

    if (send.getCalleeIdentifier() == IdentInfo_MPI_Send &&
        recv.getCalleeIdentifier() == IdentInfo_MPI_Recv) {
        matchSuccesful = send.getArgSVal(MPI_Send_idx::kCount) ==
                             recv.getArgSVal(MPI_Recv_idx::kCount) &&

                         send.getArgSVal(MPI_Send_idx::kDatatype) ==
                             recv.getArgSVal(MPI_Recv_idx::kDatatype) &&

                         send.getArgSVal(MPI_Send_idx::kDest) ==
                             recv.getArgSVal(MPI_Recv_idx::kSource) &&

                         send.getArgSVal(MPI_Send_idx::kTag) ==
                             recv.getArgSVal(MPI_Recv_idx::kTag) &&

                         send.getArgSVal(MPI_Send_idx::kComm) ==
                             recv.getArgSVal(MPI_Send_idx::kComm);

    } else {
        // assert this part of code should not be reached
        llvm_unreachable("no send/recv pair to check found");
    }

    return matchSuccesful;
}

// check for const relation
void MessageChecker::checkForMatchingSend(const CallEvent &callEvent,
                                          CheckerContext &context) const {
    ProgramStateRef progStateR = context.getState();
    StateListTy list = progStateR->get<StateList>();

    bool hasPartner{false};

    // all args are const
    if (areCommArgsConst(callEvent)) {
        for (const Message &mess : list) {
            // if unreceived MPI_Send found and all communication args const
            if (mess.callEvent_->getCalleeIdentifier() == IdentInfo_MPI_Send) {
                if (areCommArgsConst(*mess.callEvent_)) {
                    hasPartner = matchConstRecv(*mess.callEvent_, callEvent);
                }
            }

            if (hasPartner) {
                // remove matching send
                progStateR = progStateR->remove<StateList>(mess);
                context.addTransition(progStateR);
                break;
            }
        }
    }
    // at least one arg is dynamic
    else {
        for (const Message &mess : list) {
            // if unreceived MPI_Send found and all communication args const
            if (mess.callEvent_->getCalleeIdentifier() == IdentInfo_MPI_Send) {
                if (!areCommArgsConst(*mess.callEvent_)) {
                    hasPartner = matchDynRecv(*mess.callEvent_, callEvent);
                }
            }
            if (hasPartner) {
                // remove matching send
                progStateR = progStateR->remove<StateList>(mess);
                context.addTransition(progStateR);
                break;
            }
        }
    }

    if (!hasPartner) {
        reportUnmatchedRecv(callEvent, context);
    }
}

// call event: function called before this program point
// checker context state at current program point
void MessageChecker::checkPostCall(const CallEvent &callEvent,
                                   CheckerContext &context) const {
    initIdentifierInfo(context.getASTContext());
    ProgramStateRef progStateR = context.getState();
    StateListTy list = progStateR->get<StateList>();

    // MPI_Send called
    if (callEvent.getCalleeIdentifier() == IdentInfo_MPI_Send) {
        // same send currently in list -> report duplicate
        if (isIdenticalCallInList(callEvent, list)) {
            reportDuplicateSend(callEvent, context);
        } else {
            // add message to program-state
            progStateR = progStateR->add<StateList>(Message(
                callEvent.cloneWithState(progStateR), Message::State::kSent));
            context.addTransition(progStateR);
        }
        return;
    }

    // MPI_Recv called
    else if (callEvent.getCalleeIdentifier() == IdentInfo_MPI_Recv) {
        // checkFor
        checkForMatchingSend(callEvent, context);
        // blocking receive should not be added to state
    }

    return;
}

void MessageChecker::reportUnmatchedRecv(const CallEvent &callEvent,
                                         CheckerContext &context) const {
    // sink, current path hit a critical bug and is not further investigated
    ExplodedNode *ErrNode = context.generateSink();
    if (!ErrNode) return;
    BugReport *bugReport = new BugReport(*UnmatchedRecvBugType,
                                         "unmatched receive - could not "
                                         "find corresponding send",
                                         ErrNode);
    // highlight source code position where the bug occured
    bugReport->addRange(callEvent.getSourceRange());
    // fire report
    context.emitReport(bugReport);
}

void MessageChecker::reportDuplicateSend(const CallEvent &callEvent,
                                         CheckerContext &context) const {
    // sink, current path hit a critical bug and is not further investigated
    ExplodedNode *ErrNode = context.generateSink();
    if (!ErrNode) return;
    BugReport *bugReport =
        new BugReport(*DuplicateSendBugType, "duplicate send", ErrNode);
    // highlight source code position where the bug occured
    bugReport->addRange(callEvent.getSourceRange());
    // fire report
    context.emitReport(bugReport);
}

// not really needed
// covered with duplicate send & unmatched recv
void MessageChecker::reportDuplicateRecv(const CallEvent &callEvent,
                                         CheckerContext &context) const {
    // sink, current path hit a critical bug and is not further investigated
    ExplodedNode *ErrNode = context.generateSink();
    if (!ErrNode) return;
    BugReport *bugReport =
        new BugReport(*DoubleRecvBugType, "duplicate recv", ErrNode);
    // highlight source code position where the bug occured
    bugReport->addRange(callEvent.getSourceRange());
    // fire report
    context.emitReport(bugReport);
}

void ento::registerBasicMPIChecker(CheckerManager &mgr) {
    mgr.registerChecker<MessageChecker>();
}
