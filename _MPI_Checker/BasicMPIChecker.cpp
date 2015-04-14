#include "ClangSACheckers.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugType.h"
#include "clang/StaticAnalyzer/Core/Checker.h"
#include "clang/StaticAnalyzer/Core/CheckerManager.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CallEvent.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/ProgramStateTrait.h"
#include <iostream>
#include <vector>

#include "Container.hpp"
#include "Typedefs.hpp"

// checker bool: is optimistic?
// pointer escape (add state escaped?)
// call event is global function
// message state is nullptr if unknown
// -> why if is checking for nullptr first
// type of SVal can be asked

using namespace clang;
using namespace ento;

// argument schema enums ––––––––––––––––––––––––––––––––––––––
// scope enums, but keep weak typing

namespace MPIPointToPoint {
// valid for all point to point functions
enum { kBuf, kCount, kDatatype, kRank, kTag, kComm, kRequest };
}
//–––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

class Message {
private:
public:
    // states are const
    const enum class State { kSent, kRecvd } state_;

    const CallEventRef<> callEvent_;

    Message(const CallEventRef<> event, State initialState)
        : state_{initialState}, callEvent_{event} {}

    bool operator==(const Message &message) const {
        // check if all call-event args are equal
        if (callEvent_->getNumArgs() == message.callEvent_->getNumArgs()) {
            for (size_t i = 0; i < callEvent_->getNumArgs(); ++i) {
                // if one arg is different
                if (callEvent_->getArgSVal(i) !=
                    message.callEvent_->getArgSVal(i)) {
                    return false;
                }
            }
        }
        // different arg number
        else {
            return false;
        }

        // check message state
        if (state_ != message.state_) {
            return false;
        }

        return true;
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
    static std::vector<IdentifierInfo *> mpiSendTypes;
    static std::vector<IdentifierInfo *> mpiRecvTypes;

    static std::vector<IdentifierInfo *> mpiBlockingTypes;
    static std::vector<IdentifierInfo *> mpiNonBlockingTypes;

    static std::vector<IdentifierInfo *> mpiPointToPointTypes;
    static std::vector<IdentifierInfo *> mpiPointToCollTypes;
    static std::vector<IdentifierInfo *> mpiCollToPointTypes;
    static std::vector<IdentifierInfo *> mpiCollToCollTypes;

    // II -> identifier info
    mutable IdentifierInfo *IdentInfo_MPI_Send, *IdentInfo_MPI_Recv,
        *IdentInfo_MPI_Isend, *IdentInfo_MPI_Irecv, *IdentInfo_MPI_Issend,
        *IdentInfo_MPI_Ssend, *IdentInfo_MPI_Bsend, *IdentInfo_MPI_Rsend;

    // inform clang about new kind of bug types
    std::unique_ptr<BugType> DuplicateSendBugType;
    std::unique_ptr<BugType> UnmatchedRecvBugType;

    void initIdentifierInfo(ASTContext &Ctx) const;

    void reportDuplicateSend(const CallEvent &, CheckerContext &) const;
    void reportUnmatchedRecv(const CallEvent &, CheckerContext &) const;

    bool isIdenticalCallInList(const CallEvent &, StateListTy) const;
    bool areCommArgsConst(const CallEvent &) const;

    void checkForMatchingSend(const CallEvent &, CheckerContext &) const;
    bool hasMatchingSend(const CallEvent &, CheckerContext &) const;
    bool isSendRecvPairMatching(const CallEvent &, const CallEvent &) const;

    bool isSendType(const CallEvent &) const;
    bool isRecvType(const CallEvent &) const;
    bool isBlockingType(const CallEvent &) const;
    bool isNonBlockingType(const CallEvent &) const;
    bool isPointToPointType(const CallEvent &) const;
    bool isPointToCollType(const CallEvent &) const;
    bool isCollToPointType(const CallEvent &) const;
    bool isCollToCollType(const CallEvent &) const;

public:
    MessageChecker();
    void checkPostCall(const CallEvent &, CheckerContext &) const;
};

std::vector<IdentifierInfo *> MessageChecker::mpiSendTypes;
std::vector<IdentifierInfo *> MessageChecker::mpiRecvTypes;

std::vector<IdentifierInfo *> MessageChecker::mpiBlockingTypes;
std::vector<IdentifierInfo *> MessageChecker::mpiNonBlockingTypes;

std::vector<IdentifierInfo *> MessageChecker::mpiPointToPointTypes;
std::vector<IdentifierInfo *> MessageChecker::mpiPointToCollTypes;
std::vector<IdentifierInfo *> MessageChecker::mpiCollToPointTypes;
std::vector<IdentifierInfo *> MessageChecker::mpiCollToCollTypes;

MessageChecker::MessageChecker()
    : IdentInfo_MPI_Send(0), IdentInfo_MPI_Recv(0) {
    // Initialize the bug types.
    DuplicateSendBugType.reset(
        new BugType(this, "duplicate send", "MPI Error"));

    UnmatchedRecvBugType.reset(
        new BugType(this, "unmatched receive", "MPI Error"));
}

void MessageChecker::initIdentifierInfo(ASTContext &context) const {
    // only init identifiers once
    if (IdentInfo_MPI_Send) return;

    // init function identifiers
    IdentInfo_MPI_Send = &context.Idents.get("MPI_Send");
    mpiSendTypes.push_back(IdentInfo_MPI_Send);
    mpiPointToPointTypes.push_back(IdentInfo_MPI_Send);
    mpiBlockingTypes.push_back(IdentInfo_MPI_Send);

    IdentInfo_MPI_Recv = &context.Idents.get("MPI_Recv");
    mpiRecvTypes.push_back(IdentInfo_MPI_Recv);
    mpiPointToPointTypes.push_back(IdentInfo_MPI_Recv);
    mpiBlockingTypes.push_back(IdentInfo_MPI_Recv);

    IdentInfo_MPI_Isend = &context.Idents.get("MPI_Isend");
    mpiSendTypes.push_back(IdentInfo_MPI_Isend);
    mpiPointToPointTypes.push_back(IdentInfo_MPI_Isend);
    mpiNonBlockingTypes.push_back(IdentInfo_MPI_Isend);

    IdentInfo_MPI_Irecv = &context.Idents.get("MPI_Irecv");
    mpiRecvTypes.push_back(IdentInfo_MPI_Irecv);
    mpiPointToPointTypes.push_back(IdentInfo_MPI_Irecv);
    mpiNonBlockingTypes.push_back(IdentInfo_MPI_Irecv);

    IdentInfo_MPI_Ssend = &context.Idents.get("MPI_Ssend");
    mpiSendTypes.push_back(IdentInfo_MPI_Ssend);
    mpiPointToPointTypes.push_back(IdentInfo_MPI_Ssend);
    mpiBlockingTypes.push_back(IdentInfo_MPI_Ssend);

    IdentInfo_MPI_Issend = &context.Idents.get("MPI_Issend");
    mpiSendTypes.push_back(IdentInfo_MPI_Issend);
    mpiPointToPointTypes.push_back(IdentInfo_MPI_Issend);
    mpiNonBlockingTypes.push_back(IdentInfo_MPI_Issend);

    IdentInfo_MPI_Bsend = &context.Idents.get("MPI_Bsend");
    mpiSendTypes.push_back(IdentInfo_MPI_Bsend);
    mpiPointToPointTypes.push_back(IdentInfo_MPI_Bsend);
    mpiBlockingTypes.push_back(IdentInfo_MPI_Bsend);

    // validate
    IdentInfo_MPI_Rsend = &context.Idents.get("MPI_Rsend");
    mpiSendTypes.push_back(IdentInfo_MPI_Rsend);
    mpiPointToPointTypes.push_back(IdentInfo_MPI_Rsend);
    mpiBlockingTypes.push_back(IdentInfo_MPI_Rsend);
}

bool MessageChecker::isSendType(const CallEvent &callEvent) const {
    return lx::cont::isContained(mpiSendTypes, callEvent.getCalleeIdentifier());
}

bool MessageChecker::isRecvType(const CallEvent &callEvent) const {
    return lx::cont::isContained(mpiRecvTypes, callEvent.getCalleeIdentifier());
}

bool MessageChecker::isBlockingType(const CallEvent &callEvent) const {
    return lx::cont::isContained(mpiBlockingTypes,
                                 callEvent.getCalleeIdentifier());
}

bool MessageChecker::isNonBlockingType(const CallEvent &callEvent) const {
    return lx::cont::isContained(mpiNonBlockingTypes,
                                 callEvent.getCalleeIdentifier());
}

bool MessageChecker::isPointToPointType(const CallEvent &callEvent) const {
    return lx::cont::isContained(mpiPointToPointTypes,
                                 callEvent.getCalleeIdentifier());
}

bool MessageChecker::isPointToCollType(const CallEvent &callEvent) const {
    return lx::cont::isContained(mpiPointToCollTypes,
                                 callEvent.getCalleeIdentifier());
}

bool MessageChecker::isCollToPointType(const CallEvent &callEvent) const {
    return lx::cont::isContained(mpiCollToPointTypes,
                                 callEvent.getCalleeIdentifier());
}

bool MessageChecker::isCollToCollType(const CallEvent &callEvent) const {
    return lx::cont::isContained(mpiCollToCollTypes,
                                 callEvent.getCalleeIdentifier());
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
 * Checks if potentially variable arguments used for communication are const.
 *
 * @param callEvent function call to check the args for
 *
 * @return all args are const
 */
bool MessageChecker::areCommArgsConst(const CallEvent &callEvent) const {
    std::vector<unsigned char> indices;
    // set indices to check for constness based on mpi function type
    if (isPointToPointType(callEvent)) {
        indices = {MPIPointToPoint::kCount, MPIPointToPoint::kRank,
                   MPIPointToPoint::kTag};
    }

    bool areConstant{true};
    for (unsigned char idx : indices) {
        if (!callEvent.getArgSVal(idx).isConstant()) {
            areConstant = false;
            break;
        }
    }

    return areConstant;
}

/**
 *  Checks for a specific pair if send matches receive.
 *
 * @param send
 * @param recv
 * @param constness describes if send
 *
 * @return
 */
bool MessageChecker::isSendRecvPairMatching(const CallEvent &send,
                                            const CallEvent &recv) const {
    bool areSendArgsConst{areCommArgsConst(send)};
    // send/recv must be both const or dynamic
    if (areSendArgsConst != areCommArgsConst(recv)) return false;

    bool matchSuccesful{false};
    if (isSendType(send) && isRecvType(recv)) {
        bool rankMatches;
        // const case just check if destination == source
        if (areSendArgsConst) {
            rankMatches = send.getArgSVal(MPIPointToPoint::kRank) ==
                          recv.getArgSVal(MPIPointToPoint::kRank);
            std::cout << "const" << std::endl;
        } else {
            // TODO implement dyn matching
            rankMatches = send.getArgSVal(MPIPointToPoint::kRank) ==
                          recv.getArgSVal(MPIPointToPoint::kRank);
            std::cout << "dyn" << std::endl;
        }

        matchSuccesful = send.getArgSVal(MPIPointToPoint::kCount) ==
                             recv.getArgSVal(MPIPointToPoint::kCount) &&

                         send.getArgSVal(MPIPointToPoint::kDatatype) ==
                             recv.getArgSVal(MPIPointToPoint::kDatatype) &&

                         send.getArgSVal(MPIPointToPoint::kTag) ==
                             recv.getArgSVal(MPIPointToPoint::kTag) &&

                         send.getArgSVal(MPIPointToPoint::kComm) ==
                             recv.getArgSVal(MPIPointToPoint::kComm);

        matchSuccesful = rankMatches && matchSuccesful;
    }

    else {
        // reaching this part means that arguments were used incorrectely
        // -> abort static analysis
        llvm_unreachable("no send/recv pair to check found");
    }

    return matchSuccesful;
}

/**
 * Checks if there's a matching send for a recv.
 * Works with point to point pairs.
 *
 * @param recvEvent
 * @param context
 */
void MessageChecker::checkForMatchingSend(const CallEvent &recvEvent,
                                          CheckerContext &context) const {
    ProgramStateRef progStateR = context.getState();
    StateListTy list = progStateR->get<StateList>();

    // defensive checking -> only false if there's surely no match
    bool hasMatchingSend{true};

    for (const Message &mess : list) {
        // if point-to-point send operation
        if (isSendType(*mess.callEvent_) &&
            isPointToPointType(*mess.callEvent_)) {
            hasMatchingSend =
                isSendRecvPairMatching(*mess.callEvent_, recvEvent);
        }

        if (hasMatchingSend) {
            // remove matching send to omit double match for other receives
            progStateR = progStateR->remove<StateList>(mess);
            context.addTransition(progStateR);
            break;
        }
    }

    // report if there's a send missing
    if (!hasMatchingSend) reportUnmatchedRecv(recvEvent, context);
}

void MessageChecker::checkPostCall(const CallEvent &callEvent,
                                   CheckerContext &context) const {
    initIdentifierInfo(context.getASTContext());
    ProgramStateRef progStateR = context.getState();
    StateListTy list = progStateR->get<StateList>();

    // send-operation called
    if (isSendType(callEvent)) {
        // same send currently in list -> report duplicate
        if (isIdenticalCallInList(callEvent, list)) {
            reportDuplicateSend(callEvent, context);
        }

        // add message to program-state
        progStateR = progStateR->add<StateList>(Message(
            callEvent.cloneWithState(progStateR), Message::State::kSent));
        context.addTransition(progStateR);
    }

    // recv-operation called
    else if (isRecvType(callEvent)) {
        if (isPointToPointType(callEvent)) {
            checkForMatchingSend(callEvent, context);
        }
        // blocking receive should not be added to state
    }

    if (isNonBlockingType(callEvent)) {
        // TODO track request of nonblocking call
        // if memory region is dead, report missing wait as warning
    }
}

void MessageChecker::reportUnmatchedRecv(const CallEvent &callEvent,
                                         CheckerContext &context) const {
    // sink, current path hit a critical bug and is not further investigated
    ExplodedNode *ErrNode = context.generateSink();
    if (!ErrNode) return;
    BugReport *bugReport = new BugReport(*UnmatchedRecvBugType,
                                         "unmatched receive - no "
                                         "corresponding send",
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

void ento::registerBasicMPIChecker(CheckerManager &mgr) {
    mgr.registerChecker<MessageChecker>();
}
