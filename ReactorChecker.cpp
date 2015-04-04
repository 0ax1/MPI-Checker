#include "ClangSACheckers.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugType.h"
#include "clang/StaticAnalyzer/Core/Checker.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CallEvent.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"


using namespace clang;
using namespace ento;

static constexpr int ReactorStateKey{1};

class ReactorState {
private:
    // states are const
    const enum class State { kOn, kOff } state_;

public:
    ReactorState(unsigned initialState)
        : state_{static_cast<State>(initialState)} {}

    bool isOn() const { return state_ == State::kOn; }
    bool isOff() const { return state_ == State::kOff; }

    static unsigned getOn() { return static_cast<unsigned>(State::kOn); }
    static unsigned getOff() { return static_cast<unsigned>(State::kOff); }

    // passed type is reactor state
    // identity is defined with value of state_
    bool operator==(const ReactorState &reactorState) const {
        return state_ == reactorState.state_;
    }

    // used to capture the state for all points
    void Profile(llvm::FoldingSetNodeID &foldingNodeId) const {
        // the data structure (here int) used to descrive the state
        foldingNodeId.AddInteger(static_cast<unsigned>(state_));
    }
};

// class which reactorchecker inherits from specifies checker type
class ReactorChecker : public Checker<check::PostCall> {
    // II -> identifier info
    mutable IdentifierInfo *IIturnReactorOn, *IISCRAM;

    // inform clang about new kind of bug types
    std::unique_ptr<BugType> DoubleSCRAMBugType;
    std::unique_ptr<BugType> DoubleONBugType;

    void initIdentifierInfo(ASTContext &Ctx) const;

    void reportDoubleSCRAM(const CallEvent &Call, CheckerContext &C) const;
    void reportDoubleON(const CallEvent &Call, CheckerContext &C) const;

public:
    ReactorChecker();
    /// Process turnReactorOn and SCRAM
    void checkPostCall(const CallEvent &Call, CheckerContext &C) const;
};

// to capture our custom state type in the cfg with a map
REGISTER_MAP_WITH_PROGRAMSTATE(StateMap, int, ReactorState)

ReactorChecker::ReactorChecker() : IIturnReactorOn(0), IISCRAM(0) {
    // Initialize the bug types.
    DoubleSCRAMBugType.reset(
        new BugType(this, "Double SCRAM", "Nuclear Reactor API Error"));
    DoubleONBugType.reset(
        new BugType(this, "Double ON", "Nuclear Reactor API Error"));
}

void ReactorChecker::initIdentifierInfo(ASTContext &Ctx) const {
    // only init identifiers once
    if (IIturnReactorOn) return;

    // init function identifier
    IIturnReactorOn = &Ctx.Idents.get("turnReactorOn");
    IISCRAM = &Ctx.Idents.get("SCRAM");
}

// call event: function called before this program point
// checker context state at current program point
void ReactorChecker::checkPostCall(const CallEvent &callEvent,
                                   CheckerContext &context) const {
    initIdentifierInfo(context.getASTContext());
    // call event is global function
    if (!callEvent.isGlobalCFunction()) return;

    // check if called function is turnReactorOn
    if (callEvent.getCalleeIdentifier() == IIturnReactorOn) {
        // get state
        ProgramStateRef progStateR = context.getState();
        const ReactorState *reactorState =
            progStateR->get<StateMap>(ReactorStateKey);
        // reactor state is nullptr if unknown
        // -> why if is checking for nullptr first

        // check if invariant is invalidated
        if (reactorState && reactorState->isOn()) {
            // report issue
            reportDoubleON(callEvent, context);
            return;
        }

        // add transition to cfg with state on
        progStateR =
            progStateR->set<StateMap>(ReactorStateKey, ReactorState::getOn());
        // creates new edge in explosion graph
        context.addTransition(progStateR);
        return;
    }

    // check if called function is scram
    else if (callEvent.getCalleeIdentifier() == IISCRAM) {
        // get state
        ProgramStateRef progStateR = context.getState();
        const ReactorState *reactorState =
            progStateR->get<StateMap>(ReactorStateKey);
        // reactor state is nullptr if unknown
        // -> why if is checking for nullptr first

        // check if invariant is invalidated
        if (reactorState && reactorState->isOff()) {
            // report issue
            reportDoubleSCRAM(callEvent, context);
            return;
        }

        // add transition to cfg with state off
        progStateR =
            progStateR->set<StateMap>(ReactorStateKey, ReactorState::getOff());
        // creates new edge in explosion graph
        context.addTransition(progStateR);
        return;
    }
}

void ReactorChecker::reportDoubleON(const CallEvent &callEvent,
                                    CheckerContext &context) const {
    // sink, current path hit a critical bug and is not further investigated
    ExplodedNode *ErrNode = context.generateSink();
    if (!ErrNode) return;
    BugReport *bugReport = new BugReport(
        *DoubleONBugType, "Turned on the reactor two times", ErrNode);
    // highlight source code position where the bug occured
    bugReport->addRange(callEvent.getSourceRange());
    // fire report
    context.emitReport(bugReport);
}

void ReactorChecker::reportDoubleSCRAM(const CallEvent &callEvent,
                                       CheckerContext &context) const {
    // sink, current path hit a critical bug and is not further investigated
    ExplodedNode *ErrNode = context.generateSink();
    if (!ErrNode) return;
    BugReport *bugReport = new BugReport(
        *DoubleSCRAMBugType, "Called a SCRAM procedure twice", ErrNode);
    // highlight source code position where the bug occured
    bugReport->addRange(callEvent.getSourceRange());
    // fire report
    context.emitReport(bugReport);
}

void ento::registerReactorChecker(CheckerManager &mgr) {
    mgr.registerChecker<ReactorChecker>();
}
