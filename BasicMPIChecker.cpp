#include <iostream>
#include "ClangSACheckers.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugType.h"
// #include "clang/StaticAnalyzer/Core/Checker.h"
// #include "clang/StaticAnalyzer/Core/CheckerManager.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CallEvent.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"
// #include "clang/StaticAnalyzer/Core/PathSensitive/ProgramStateTrait.h"
#include "llvm/ADT/ImmutableMap.h"

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
    // states are const
    const enum class State { kSent, kRecvd } state_;

public:
    Message(unsigned initialState) : state_{static_cast<State>(initialState)} {}

    bool isSent() const { return state_ == State::kSent; }
    bool isRecvd() const { return state_ == State::kRecvd; }

    static bool getSent() { return static_cast<bool>(State::kSent); }
    static bool getRecvd() { return static_cast<bool>(State::kRecvd); }

    // passed type is message state
    // identity is defined with value of state_
    bool operator==(const Message &message) const {
        return state_ == message.state_;
    }

    // useful to check if nodes are in the same execution state
    void Profile(llvm::FoldingSetNodeID &foldingNodeId) const {
        // the data structure (here int) used to descrive the state
        foldingNodeId.AddBoolean(static_cast<bool>(state_));
    }
};

// class which messagechecker inherits from specifies checker type
class MessageChecker : public Checker<check::PostCall> {
    // II -> identifier info
    mutable IdentifierInfo *IdentInfo_MPI_Send, *IdentInfo_MPI_Recv;

    // inform clang about new kind of bug types
    std::unique_ptr<BugType> DoubleRecvBugType;
    std::unique_ptr<BugType> DoubleSendBugType;
    std::unique_ptr<BugType> NoMatchingSendBugType;

    void initIdentifierInfo(ASTContext &Ctx) const;

    void reportDoubleRecv(const CallEvent &, CheckerContext &) const;
    void reportDoubleSend(const CallEvent &, CheckerContext &) const;
    void reportNoMatchingSend(const CallEvent &, CheckerContext &) const;

public:
    MessageChecker();
    void checkPostCall(const CallEvent &, CheckerContext &) const;
};

// to capture custom state type in the cfg with a list
REGISTER_MAP_WITH_PROGRAMSTATE(StateMap, SymbolRef, Message)

MessageChecker::MessageChecker()
    : IdentInfo_MPI_Send(0), IdentInfo_MPI_Recv(0) {
    // Initialize the bug types.
    DoubleRecvBugType.reset(
        new BugType(this, "unmatched receive", "MPI Error"));
    DoubleSendBugType.reset(new BugType(this, "double receive", "MPI Error"));
    NoMatchingSendBugType.reset(
        new BugType(this, "no matching MPI_Send", "MPI Error"));
}

void MessageChecker::initIdentifierInfo(ASTContext &Ctx) const {
    // only init identifiers once
    if (IdentInfo_MPI_Send) return;

    // init function identifier
    IdentInfo_MPI_Send = &Ctx.Idents.get("MPI_Send");
    IdentInfo_MPI_Recv = &Ctx.Idents.get("MPI_Recv");
}

// call event: function called before this program point
// checker context state at current program point
void MessageChecker::checkPostCall(const CallEvent &callEvent,
                                   CheckerContext &context) const {
    initIdentifierInfo(context.getASTContext());
    ProgramStateRef progStateR = context.getState();

    // MPI_Send called
    if (callEvent.getCalleeIdentifier() == IdentInfo_MPI_Send) {
        SymbolRef tag = callEvent.getArgSVal(4).getAsSymbol();
        auto tag2 =
            callEvent.getArgSVal(4).getAs<loc::ConcreteInt>().getValue();

        std::cout << "symbolRef: " << tag << std::endl;
        progStateR = progStateR->set<StateMap>(tag, Message::getSent());

        // std::cout <<  << std::endl;

        // std::cout << callEvent.getArgSVal(4).isUndef() << std::endl;
        // std::cout << callEvent.getArgSVal(4).isValid() << std::endl;
        // std::cout << callEvent.getArgSVal(4).isUnknown() << std::endl;
        // std::cout << callEvent.getArgSVal(4).isConstant() << std::endl;

        context.addTransition(progStateR);
        return;
    }

    // MPI_Recv called
    else if (callEvent.getCalleeIdentifier() == IdentInfo_MPI_Recv) {
        SymbolRef tag = callEvent.getArgSVal(4).getAsSymbol();
        // std::cout << tag << std::endl;
        const Message *message = progStateR->get<StateMap>(tag);

        // is nullptr if element is not contained in map
        if (message == nullptr) {
            // not matching function ever called
            reportNoMatchingSend(callEvent, context);
        }

        // check if invariant is invalidated
        if (message && message->isRecvd()) {
            // report issue
            // reportDoubleRecv(callEvent, context);
            // return;
        }

        // TODO if received
        // progStateR = progStateR->set<StateMap>(tag, Message::getRecvd());
        context.addTransition(progStateR);
        return;
    }
}

void MessageChecker::reportDoubleSend(const CallEvent &callEvent,
                                      CheckerContext &context) const {
    // sink, current path hit a critical bug and is not further investigated
    ExplodedNode *ErrNode = context.generateSink();
    if (!ErrNode) return;
    BugReport *bugReport =
        new BugReport(*DoubleSendBugType, "MPI_Send called twice.", ErrNode);
    // highlight source code position where the bug occured
    bugReport->addRange(callEvent.getSourceRange());
    // fire report
    context.emitReport(bugReport);
}

void MessageChecker::reportNoMatchingSend(const CallEvent &callEvent,
                                          CheckerContext &context) const {
    // sink, current path hit a critical bug and is not further investigated
    ExplodedNode *ErrNode = context.generateSink();
    if (!ErrNode) return;
    BugReport *bugReport =
        new BugReport(*NoMatchingSendBugType, "no matching MPI_Send.", ErrNode);
    // highlight source code position where the bug occured
    bugReport->addRange(callEvent.getSourceRange());
    // fire report
    context.emitReport(bugReport);
}

void MessageChecker::reportDoubleRecv(const CallEvent &callEvent,
                                      CheckerContext &context) const {
    // sink, current path hit a critical bug and is not further investigated
    ExplodedNode *ErrNode = context.generateSink();
    if (!ErrNode) return;
    BugReport *bugReport = new BugReport(
        *DoubleRecvBugType, "No matching MPI_Send for MPI_Recv.", ErrNode);
    // highlight source code position where the bug occured
    bugReport->addRange(callEvent.getSourceRange());
    // fire report
    context.emitReport(bugReport);
}

void ento::registerBasicMPIChecker(CheckerManager &mgr) {
    mgr.registerChecker<MessageChecker>();
}
