#include "../ClangSACheckers.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugType.h"
#include "clang/StaticAnalyzer/Core/Checker.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CallEvent.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"
#include "clang/AST/RecursiveASTVisitor.h"

#include <iostream>
#include <vector>

#include "Container.hpp"

// http://clang.llvm.org/doxygen/CheckerDocumentation_8cpp_source.html
// checkEndFunction
// checkEndAnalysis
// checkEndOfTranslationUnit

using namespace clang;
using namespace ento;

// argument schema enums ––––––––––––––––––––––––––––––––––––––
// scope enums, but keep weak typing
namespace MPIPointToPoint {
// valid for all point to point functions
enum { kBuf, kCount, kDatatype, kRank, kTag, kComm, kRequest };
}

namespace MPI_Comm_rank {
enum { kComm, kRank };
}
//–––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

/**
 * MPI function call wrapper class. Enables capturing in llvm
 * program state container classes (list, set, map).
 */
class MPIFunctionCall {
private:
public:
    // capture mpi function call with arguments
    const CallEventRef<> callEvent_;

    MPIFunctionCall(const CallEventRef<> event) : callEvent_{event} {}

    bool operator==(const MPIFunctionCall &message) const {
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
        // has different arg count
        else {
            return false;
        }

        return true;
    }

    bool operator<(const MPIFunctionCall &message) const {
        // order is irrelevant
        return true;
    }

    // to enable analyzer to check if nodes are in the same execution state
    void Profile(llvm::FoldingSetNodeID &foldingNodeId) const {
        foldingNodeId.AddPointer(&callEvent_);
    }
};

struct RankVar {
    loc::MemRegionVal sval_;

    RankVar(loc::MemRegionVal symbolRef) : sval_{symbolRef} {}

    bool operator==(const RankVar &otherRankVar) const {
        return sval_ == otherRankVar.sval_;
    }

    bool operator<(const RankVar &otherRankVar) const {
        // order is irrelevant
        return true;
    }

    void Profile(llvm::FoldingSetNodeID &ID) const { ID.AddPointer(&sval_); }
};

// capture mpi function calls in control flow graph
// operations are customly removed from cfg when completed
REGISTER_SET_WITH_PROGRAMSTATE(MPIFnCallSet, MPIFunctionCall)

// TODO
// REGISTER_TRAIT_WITH_PROGRAMSTATE(InsideRankBranch, bool)

// track rank variables set by MPI_Comm_rank
REGISTER_SET_WITH_PROGRAMSTATE(RankVarsSet, RankVar)

// REGISTER_TRAIT_WITH_PROGRAMSTATE(MPIRank, SVal)
// register if current in if-stmt

// template inheritance arguments set callback functions
class MPISchemaChecker
    // IfStmt not triggered with pre/post-stmt
    : public Checker<check::PreCall, check::PostCall, check::DeadSymbols,
                     check::BranchCondition, check::PostStmt<DeclStmt>,
                     check::Location, check::EndFunction, check::EndAnalysis,
                     check::Bind> {
public:
    // to enable classification of mpi-functions during analysis
    std::vector<IdentifierInfo *> mpiSendTypes;
    std::vector<IdentifierInfo *> mpiRecvTypes;

    std::vector<IdentifierInfo *> mpiBlockingTypes;
    std::vector<IdentifierInfo *> mpiNonBlockingTypes;

    std::vector<IdentifierInfo *> mpiPointToPointTypes;
    std::vector<IdentifierInfo *> mpiPointToCollTypes;
    std::vector<IdentifierInfo *> mpiCollToPointTypes;
    std::vector<IdentifierInfo *> mpiCollToCollTypes;
    //––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

    IdentifierInfo *IdentInfo_MPI_Send, *IdentInfo_MPI_Recv,
        *IdentInfo_MPI_Isend, *IdentInfo_MPI_Irecv, *IdentInfo_MPI_Issend,
        *IdentInfo_MPI_Ssend, *IdentInfo_MPI_Bsend, *IdentInfo_MPI_Rsend,
        *IdentInfo_MPI_Comm_rank, *IdentInfoTrackMem;

    // custom bug types
    std::unique_ptr<BugType> DuplicateSendBugType;
    std::unique_ptr<BugType> UnmatchedRecvBugType;

    void dynamicIdentifierInit(ASTContext &);
    void dynamicIdentifierInit(CheckerContext &) const;

    void reportDuplicateSend(const CallEvent &, CheckerContext &) const;
    void reportUnmatchedRecv(const CallEvent &, CheckerContext &) const;

    bool identicalMPICall(const CallEvent &, MPIFnCallSetTy) const;
    bool areCommArgsConst(const CallEvent &) const;

    void checkForMatchingSend(const CallEvent &, CheckerContext &) const;
    bool hasMatchingSend(const CallEvent &, CheckerContext &) const;
    bool isSendRecvPairMatching(const CallEvent &, const CallEvent &) const;

    void checkPreCall(const CallEvent &, CheckerContext &) const;
    void checkPostCall(const CallEvent &, CheckerContext &) const;
    void checkPostStmt(const ReturnStmt *, CheckerContext &) const;
    void checkPostStmt(const DeclStmt *, CheckerContext &) const;
    void checkEndFunction(CheckerContext &) const;
    void checkEndAnalysis(ExplodedGraph &, BugReporter &, ExprEngine &) const;
    void checkBind(SVal, SVal, const Stmt *, CheckerContext &) const;

    void checkBranchCondition(const Stmt *, CheckerContext &) const;

    ProgramStateRef checkPointerEscape(ProgramStateRef,
                                       const InvalidatedSymbols &,
                                       const CallEvent *,
                                       PointerEscapeKind) const;

    void checkDeadSymbols(SymbolReaper &, CheckerContext &) const;
    void checkLocation(SVal, bool, const Stmt *, CheckerContext &) const;

    // to inspect properties of mpi functions
    bool isSendType(const CallEvent &) const;
    bool isRecvType(const CallEvent &) const;
    bool isBlockingType(const CallEvent &) const;
    bool isNonBlockingType(const CallEvent &) const;
    bool isPointToPointType(const CallEvent &) const;
    bool isPointToCollType(const CallEvent &) const;
    bool isCollToPointType(const CallEvent &) const;
    bool isCollToCollType(const CallEvent &) const;

    void memRegionInfo(const MemRegion *MR) const;

    MPISchemaChecker()
        :  // inspecting the state of initialization is based
           // on MPI_Send identifier pointer (see dynamicIdentifierInit)
          IdentInfo_MPI_Send(nullptr) {
        // initialize bug types
        DuplicateSendBugType.reset(
            new BugType(this, "duplicate send", "MPI Error"));

        UnmatchedRecvBugType.reset(
            new BugType(this, "unmatched receive", "MPI Error"));
    };
};

/**
 * Initializes function identifiers lazily. This is the default pattern
 * for initializing checker identifiers. Instead of using strings,
 * indentifier-pointers are initially captured to recognize functions during
 * analysis by comparison later.
 *
 * @param context that is used for analyzing cfg nodes
 */
void MPISchemaChecker::dynamicIdentifierInit(ASTContext &context) {
    // guard to check if identifiers are intialized
    if (IdentInfo_MPI_Send) return;

    // init function identifiers
    // and copy them into the correct classification containers
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

    // non communicating functions
    IdentInfo_MPI_Comm_rank = &context.Idents.get("MPI_Comm_rank");

    IdentInfoTrackMem = &context.Idents.get("trackMem");
}

/**
 * Convenience function to enable dynamic initialization triggered
 * from const functions. Initializes identifiers lazily. This is the
 * default pattern for initializating checker identifier.
 *
 * @param context
 */
void MPISchemaChecker::dynamicIdentifierInit(CheckerContext &context) const {
    if (IdentInfo_MPI_Send) return;
    const_cast<MPISchemaChecker *>(this)
        ->dynamicIdentifierInit(context.getASTContext());
}

/**
 * Check if MPI send function
 */
bool MPISchemaChecker::isSendType(const CallEvent &callEvent) const {
    return lx::cont::isContained(mpiSendTypes, callEvent.getCalleeIdentifier());
}

/**
 * Check if MPI recv function
 */
bool MPISchemaChecker::isRecvType(const CallEvent &callEvent) const {
    return lx::cont::isContained(mpiRecvTypes, callEvent.getCalleeIdentifier());
}

/**
 * Check if MPI blocking function
 */
bool MPISchemaChecker::isBlockingType(const CallEvent &callEvent) const {
    return lx::cont::isContained(mpiBlockingTypes,
                                 callEvent.getCalleeIdentifier());
}

/**
 * Check if MPI nonblocking function
 */
bool MPISchemaChecker::isNonBlockingType(const CallEvent &callEvent) const {
    return lx::cont::isContained(mpiNonBlockingTypes,
                                 callEvent.getCalleeIdentifier());
}

/**
 * Check if MPI point to point function
 */
bool MPISchemaChecker::isPointToPointType(const CallEvent &callEvent) const {
    return lx::cont::isContained(mpiPointToPointTypes,
                                 callEvent.getCalleeIdentifier());
}

/**
 * Check if MPI point to collective function
 */
bool MPISchemaChecker::isPointToCollType(const CallEvent &callEvent) const {
    return lx::cont::isContained(mpiPointToCollTypes,
                                 callEvent.getCalleeIdentifier());
}

/**
 * Check if MPI collective to point function
 */
bool MPISchemaChecker::isCollToPointType(const CallEvent &callEvent) const {
    return lx::cont::isContained(mpiCollToPointTypes,
                                 callEvent.getCalleeIdentifier());
}

/**
 * Check if MPI collective to collective function
 */
bool MPISchemaChecker::isCollToCollType(const CallEvent &callEvent) const {
    return lx::cont::isContained(mpiCollToCollTypes,
                                 callEvent.getCalleeIdentifier());
}

void MPISchemaChecker::memRegionInfo(const MemRegion *memRegion) const {

    llvm::SmallString<100> buf;
    llvm::raw_svector_ostream os(buf);

    switch (memRegion->getKind()) {
        case MemRegion::FunctionTextRegionKind: {
            const NamedDecl *FD =
                cast<FunctionTextRegion>(memRegion)->getDecl();
            if (FD)
                os << "the address of the function '" << *FD << '\'';
            else
                os << "the address of a function";
        }
        case MemRegion::BlockTextRegionKind:
            os << "block text";
        case MemRegion::BlockDataRegionKind:
            os << "a block";
        default: {
            const MemSpaceRegion *MS = memRegion->getMemorySpace();

            if (isa<StackLocalsSpaceRegion>(MS)) {
                const VarRegion *VR = dyn_cast<VarRegion>(memRegion);
                const VarDecl *VD;
                if (VR)
                    VD = VR->getDecl();
                else
                    VD = nullptr;

                if (VD)
                    os << "the address of the local variable '" << VD->getName()
                       << "'";
                else
                    os << "the address of a local stack variable";
            }

            if (isa<StackArgumentsSpaceRegion>(MS)) {
                const VarRegion *VR = dyn_cast<VarRegion>(memRegion);
                const VarDecl *VD;
                if (VR)
                    VD = VR->getDecl();
                else
                    VD = nullptr;

                if (VD)
                    os << "the address of the parameter '" << VD->getName()
                       << "'";
                else
                    os << "the address of a parameter";
            }

            if (isa<GlobalsSpaceRegion>(MS)) {
                const VarRegion *VR = dyn_cast<VarRegion>(memRegion);
                const VarDecl *VD;
                if (VR)
                    VD = VR->getDecl();
                else
                    VD = nullptr;

                if (VD) {
                    if (VD->isStaticLocal())
                        os << "the address of the static variable '"
                           << VD->getName() << "'";
                    else
                        os << "the address of the global variable '"
                           << VD->getName() << "'";
                } else
                    os << "the address of a global variable";
            }
        }
    }
    std::cout << os.str().str() << std::endl;
}

/**
 * Check if the exact same call is already in the MPI function call set.
 * (ty = type)
 *
 * @param callEvent
 * @param mpiFnCallSet set searched for identical calls
 *
 * @return is equal call in list
 */
bool MPISchemaChecker::identicalMPICall(const CallEvent &callEvent,
                                        MPIFnCallSetTy mpiFnCallSet) const {
    bool identical{false};

    // check for identical call
    for (const MPIFunctionCall &mess : mpiFnCallSet) {
        // if calls have the same identifier - implies same number of args
        if (mess.callEvent_->getCalleeIdentifier() ==
            callEvent.getCalleeIdentifier()) {
            identical = true;
            const size_t numArgs{callEvent.getNumArgs()};
            for (size_t i = 0; i < numArgs; ++i) {
                if (callEvent.getArgSVal(i) != mess.callEvent_->getArgSVal(i)) {
                    // call not identical, check next
                    identical = false;
                    break;
                }
            }
            // end if identical call was found
            if (identical) break;
        }
    }
    return identical;
}

/**
 * Checks if potentially variable arguments used for communication are const.
 *
 * @param callEvent function to check args for
 *
 * @return all args are const
 */
bool MPISchemaChecker::areCommArgsConst(const CallEvent &callEvent) const {
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
 * Checks for a specific pair if send matches receive.
 *
 * @param send
 * @param recv
 * @param constness describes if send
 *
 * @return
 */
bool MPISchemaChecker::isSendRecvPairMatching(const CallEvent &send,
                                              const CallEvent &recv) const {
    bool areSendArgsConst{areCommArgsConst(send)};
    // send/recv must be both const or dynamic
    // if (areSendArgsConst != areCommArgsConst(recv)) return false;

    bool matchSuccesful{false};
    if (isSendType(send) && isRecvType(recv)) {
        bool rankMatches;
        // const case: just check if destination == source
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

// void MPISchemaChecker::checkPostStmt(const ReturnStmt *S,
// CheckerContext &C) const {
// std::cout << "pre return" << std::endl;
// }

void MPISchemaChecker::checkPostStmt(const ReturnStmt *S,
                                     CheckerContext &C) const {}

/* We specifically ignore loop conditions, because they're typically
 not error checks.  */
// void VisitWhileStmt(WhileStmt *S) {
// return this->Visit(S->getBody());
// }
// void VisitForStmt(ForStmt *S) {
// return this->Visit(S->getBody());
// }
// void VisitDoStmt(DoStmt *S) {
// return this->Visit(S->getBody());
// }

/**
 * Checks if there's a matching send for a recv.
 * Works with point to point pairs.
 *
 * @param recvEvent
 * @param context
 */
void MPISchemaChecker::checkForMatchingSend(const CallEvent &recvEvent,
                                            CheckerContext &context) const {
    ProgramStateRef progStateR = context.getState();
    MPIFnCallSetTy mpiFunctionCalls = progStateR->get<MPIFnCallSet>();

    // defensive checking -> only false if there's surely no match
    bool hasMatchingSend{false};

    for (const MPIFunctionCall &mpiFunctionCall : mpiFunctionCalls) {
        // if point-to-point send operation
        if (isSendType(*mpiFunctionCall.callEvent_) &&
            isPointToPointType(*mpiFunctionCall.callEvent_)) {
            hasMatchingSend =
                isSendRecvPairMatching(*mpiFunctionCall.callEvent_, recvEvent);
        }

        if (hasMatchingSend) {
            // remove matching send to omit double match for other receives
            progStateR = progStateR->remove<MPIFnCallSet>(mpiFunctionCall);
            context.addTransition(progStateR);
            break;
        }
    }

    // report if there's a send missing
    if (!hasMatchingSend) reportUnmatchedRecv(recvEvent, context);
}

void MPISchemaChecker::checkDeadSymbols(SymbolReaper &symbolReaper,
                                        CheckerContext &context) const {
    if (!symbolReaper.hasDeadSymbols()) return;
}

void MPISchemaChecker::checkPreCall(const CallEvent &callEvent,
                                    CheckerContext &context) const {
    dynamicIdentifierInit(context);

    ProgramStateRef progStateR = context.getState();
    MPIFnCallSetTy mpiCalls = progStateR->get<MPIFnCallSet>();

    // send-operation called
    if (isSendType(callEvent)) {
        // same send currently in list -> report duplicate
        if (identicalMPICall(callEvent, mpiCalls)) {
            reportDuplicateSend(callEvent, context);
        }

        // add message to program-state
        progStateR = progStateR->add<MPIFnCallSet>(
            MPIFunctionCall(callEvent.cloneWithState(progStateR)));
        context.addTransition(progStateR);
    }

    // recv-operation called
    else if (isRecvType(callEvent)) {
        // TODO
        // collect mpi calls if inside if/else block

        if (isPointToPointType(callEvent)) {
            checkForMatchingSend(callEvent, context);
        }
        // blocking receive should not be added to state
        // doch falls in if stmt
    }

    if (isNonBlockingType(callEvent)) {
        // TODO track request of nonblocking call
        // if memory region is dead, report missing wait as warning
    }
}

void MPISchemaChecker::checkPostCall(const CallEvent &callEvent,
                                     CheckerContext &context) const {
    dynamicIdentifierInit(context);

    // callEvent.dump();

    // track rank variables
    if (callEvent.getCalleeIdentifier() == IdentInfo_MPI_Comm_rank) {
        ProgramStateRef progStateRef = context.getState();
        SVal rankVarSVal = callEvent.getArgSVal(MPI_Comm_rank::kRank);

        const MemRegion *MR = rankVarSVal.getAsRegion();
        memRegionInfo(MR);

        loc::MemRegionVal X = rankVarSVal.castAs<loc::MemRegionVal>();
        if (!progStateRef->contains<RankVarsSet>(X)) {
            progStateRef = progStateRef->add<RankVarsSet>(X);
            context.addTransition(progStateRef);

        } else {
            std::cout << "reuse rank var" << std::endl;
        }
    }
}

class FindNamedClassVisitor
    : public RecursiveASTVisitor<FindNamedClassVisitor> {
public:
    bool VisitDecl(Decl *declaration) {
        // For debugging, dumping the AST nodes will show which nodes are
        // already
        // being visited.
        declaration->dump();

        // The return value indicates whether we want the visitation to proceed.
        // Return false to stop the traversal of the AST.
        return true;
    }

    bool VisitExpr(Expr *expression) {
        // For debugging, dumping the AST nodes will show which nodes are
        // already
        // being visited.
        expression->dump();

        // The return value indicates whether we want the visitation to proceed.
        // Return false to stop the traversal of the AST.
        return true;
    }

    bool VisitDeclRefExpr(DeclRefExpr *expression) {
        // For debugging, dumping the AST nodes will show which nodes are
        // already
        // being visited.
        expression->dump();

        // The return value indicates whether we want the visitation to proceed.
        // Return false to stop the traversal of the AST.
        return true;
    }
};

void MPISchemaChecker::checkBranchCondition(const Stmt *condition,
                                            CheckerContext &ctx) const {
    // condition->dumpColor();
    if (const BinaryOperator *b = dyn_cast<BinaryOperator>(condition)) {
        if (b->isComparisonOp()) {
            Expr *LHS = b->getLHS();
            SVal val = ctx.getSVal(LHS);

            const MemRegion *MR = val.getAsRegion();
            if (MR) {
                memRegionInfo(MR);
            } else {
                std::cout << "nullptr region" << std::endl;
            }

            // SVal val = ctx.getSVal(Val.getAsRegion());

            // FindNamedClassVisitor ncv;
            // ncv.VisitExpr(LHS);

            // ProgramStateRef progStateRef = ctx.getState();

            // if (progStateRef->contains<RankVarsSet>(Val)) {
            // std::cout << "found in branch" << std::endl;
            // }
        }
    }
}

void MPISchemaChecker::checkEndFunction(CheckerContext &context) const {
    // context.getPredecessor()->getLocationContext()->dumpStack
    // context.set
    // auto &con = context.getASTContext();
}

void MPISchemaChecker::checkBind(SVal Loc, SVal val, const Stmt *statement,
                                 CheckerContext &context) const {
    ProgramStateRef progStateRef = context.getState();
    // if (progStateRef->contains<RankVarsSet>(Loc)) {
    // std::cout << "reassigned" << std::endl;
    // }

    // statement->dumpColor();
    if (const BinaryOperator *b = dyn_cast<BinaryOperator>(statement)) {
        if (b->isComparisonOp()) {
            std::cout << "binary comp" << std::endl;
        }
    }
    // if (const auto *b = dyn_cast<VarDecl>(statement)) {
    // std::cout << "declrefexpr" << std::endl;
    // }

    // statement->dump();
    // ProgramStateRef progStateRef = context.getState();
    // if (const DeclStmt *declS = dyn_cast<DeclStmt>(statement)) {

    // for (const auto &x : declS->decls()) {
    // if (const auto v = dyn_cast<VarDecl>(x)->i) {
    // std::cout << v->getName().str() << std::endl;

    // auto s = v->getInit();
    // v->getInit()->getSourceBitField
    // Expr *ex = v->getInit();
    // ex->hasAnyTypeDependentArguments
    // }
    // }
    // if (progStateRef->contains<RankVarsSet>(v)) {
    // std::cout << "rank var bound" << std::endl;
    // context.addTransition(progStateRef);
    // }
    // }

    // if (const BinaryOperator *b = dyn_cast<BinaryOperator>(statement)) {
    // std::cout << "binary" << std::endl;
    // SVal rhsSVal = progStateRef->getSVal(b->getRHS(),
    // context.getLocationContext());

    // std::cout << "assign" << std::endl;

    // if (progStateRef->contains<RankVarsSet>(rhsSVal)) {
    // std::cout << "rank var bound" << std::endl;
    // // context.addTransition(progStateRef);
    // }
    // }
}

void MPISchemaChecker::checkPostStmt(const DeclStmt *DS,
                                     CheckerContext &C) const {
    // std::cout << "post" << std::endl;
    // if (const UnaryOperator * u = dyn_cast<UnaryOperator>(DS)) {
    // std::cout << "unary" << std::endl;
    // }
    // if (const BinaryOperator * u = dyn_cast<BinaryOperator>(DS)) {
    // std::cout << "binary" << std::endl;
    // }
}

void MPISchemaChecker::checkEndAnalysis(ExplodedGraph &explodedGraph,
                                        BugReporter &bugReporter,
                                        ExprEngine &expressionEngine) const {
    // expressionEngine.ViewGraph(0);
}

void MPISchemaChecker::reportUnmatchedRecv(const CallEvent &callEvent,
                                           CheckerContext &context) const {
    // sink, current path hit a critical bug and is not further investigated
    ExplodedNode *ErrNode = context.generateSink();
    // ErrNode->getFirstPred
    // ErrNode->addPredecessor
    if (!ErrNode) return;
    BugReport *bugReport = new BugReport(*UnmatchedRecvBugType,
                                         "unmatched receive - no "
                                         "corresponding send",
                                         ErrNode);
    // highlight source code position
    bugReport->addRange(callEvent.getSourceRange());
    // report
    context.emitReport(bugReport);
}

void MPISchemaChecker::reportDuplicateSend(const CallEvent &callEvent,
                                           CheckerContext &context) const {
    // sink, current path hit a critical bug and is not further investigated
    ExplodedNode *ErrNode = context.generateSink();
    if (!ErrNode) return;
    BugReport *bugReport =
        new BugReport(*DuplicateSendBugType, "duplicate send", ErrNode);
    // highlight source code position
    bugReport->addRange(callEvent.getSourceRange());
    // report
    context.emitReport(bugReport);
}

void MPISchemaChecker::checkLocation(SVal Loc, bool IsLoad, const Stmt *S,
                                     CheckerContext &ctx) const {

    // S->dumpColor();
    // memRegionInfo(Loc.getAsRegion());
}

void ento::registerMPISchemaChecker(CheckerManager &mgr) {
    mgr.registerChecker<MPISchemaChecker>();
}
