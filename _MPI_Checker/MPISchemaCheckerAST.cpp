#include "../ClangSACheckers.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugType.h"
#include "clang/StaticAnalyzer/Core/Checker.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CallEvent.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"
#include "clang/AST/RecursiveASTVisitor.h"

#include <iostream>
#include <vector>
#include <functional>

#include "Container.hpp"

// callback functions a checker can register for
// http://clang.llvm.org/doxygen/CheckerDocumentation_8cpp_source.html

// dump-color legend
// Red           - CastColor
// Green         - TypeColor
// Bold Green    - DeclKindNameColor, UndeserializedColor
// Yellow        - AddressColor, LocationColor
// Blue          - CommentColor, NullColor, IndentColor
// Bold Blue     - AttrColor
// Bold Magenta  - StmtColor
// Cyan          - ValueKindColor, ObjectKindColor
// Bold Cyan     - ValueColor, DeclNameColor

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

// TODO subclass nur für identifier info erstellen?

class MPI_ASTVisitor : public RecursiveASTVisitor<MPI_ASTVisitor> {
    const BugReporter &bugReporter_;
    const CheckerBase &checkerBase_;
    const AnalysisDeclContext &analysisDeclContext_;

public:
    MPI_ASTVisitor(const BugReporter &bugReporter,
                   const CheckerBase &checkerBase,
                   const AnalysisDeclContext &analysisDeclContext)
        : bugReporter_{bugReporter},
          checkerBase_{checkerBase},
          analysisDeclContext_{analysisDeclContext} {
        identifierInit(analysisDeclContext.getASTContext());
    }

    // to enable classification of mpi-functions during analysis
    std::vector<IdentifierInfo *> mpiSendTypes;
    std::vector<IdentifierInfo *> mpiRecvTypes;

    std::vector<IdentifierInfo *> mpiBlockingTypes;
    std::vector<IdentifierInfo *> mpiNonBlockingTypes;

    std::vector<IdentifierInfo *> mpiPointToPointTypes;
    std::vector<IdentifierInfo *> mpiPointToCollTypes;
    std::vector<IdentifierInfo *> mpiCollToPointTypes;
    std::vector<IdentifierInfo *> mpiCollToCollTypes;

    IdentifierInfo *identInfo_MPI_Send{nullptr}, *identInfo_MPI_Recv{nullptr},
        *identInfo_MPI_Isend{nullptr}, *identInfo_MPI_Irecv{nullptr},
        *identInfo_MPI_Issend{nullptr}, *identInfo_MPI_Ssend{nullptr},
        *identInfo_MPI_Bsend{nullptr}, *identInfo_MPI_Rsend{nullptr},
        *identInfo_MPI_Comm_rank{nullptr}, *IdentInfoTrackMem{nullptr};

    // custom bug types
    std::unique_ptr<BugType> DuplicateSendBugType;
    std::unique_ptr<BugType> UnmatchedRecvBugType;

    void identifierInit(ASTContext &);

    // visitor callbacks
    bool VisitDecl(Decl *);
    bool VisitFunctionDecl(FunctionDecl *);
    bool VisitDeclRefExpr(DeclRefExpr *);
    bool VisitCallExpr(CallExpr *);
    // TODO
    // bool VisitIfStmt(const IfStmt *);

    // to inspect properties of mpi functions
    bool isSendType(const IdentifierInfo *) const;
    bool isRecvType(const IdentifierInfo *) const;
    bool isBlockingType(const IdentifierInfo *) const;
    bool isNonBlockingType(const IdentifierInfo *) const;
    bool isPointToPointType(const IdentifierInfo *) const;
    bool isPointToCollType(const IdentifierInfo *) const;
    bool isCollToPointType(const IdentifierInfo *) const;
    bool isCollToCollType(const IdentifierInfo *) const;

    const Type *getType(const DeclRefExpr *) const;
    const Type *getType(const CallExpr *, size_t) const;

    bool duplicateMPICall(CallExpr *) const;
};

// TODO copy maybe incorrect
std::vector<CallExpr *> mpiCalls;

/**
 * Initializes function identifiers lazily. This is the default pattern
 * for initializing checker identifiers. Instead of using strings,
 * indentifier-pointers are initially captured to recognize functions during
 * analysis by comparison later.
 *
 * @param context that is used for analyzing cfg nodes
 */
void MPI_ASTVisitor::identifierInit(ASTContext &context) {
    // init function identifiers
    // and copy them into the correct classification containers
    identInfo_MPI_Send = &context.Idents.get("MPI_Send");
    mpiSendTypes.push_back(identInfo_MPI_Send);
    mpiPointToPointTypes.push_back(identInfo_MPI_Send);
    mpiBlockingTypes.push_back(identInfo_MPI_Send);
    assert(identInfo_MPI_Send);

    identInfo_MPI_Recv = &context.Idents.get("MPI_Recv");
    mpiRecvTypes.push_back(identInfo_MPI_Recv);
    mpiPointToPointTypes.push_back(identInfo_MPI_Recv);
    mpiBlockingTypes.push_back(identInfo_MPI_Recv);
    assert(identInfo_MPI_Recv);

    identInfo_MPI_Isend = &context.Idents.get("MPI_Isend");
    mpiSendTypes.push_back(identInfo_MPI_Isend);
    mpiPointToPointTypes.push_back(identInfo_MPI_Isend);
    mpiNonBlockingTypes.push_back(identInfo_MPI_Isend);
    assert(identInfo_MPI_Isend);

    identInfo_MPI_Irecv = &context.Idents.get("MPI_Irecv");
    mpiRecvTypes.push_back(identInfo_MPI_Irecv);
    mpiPointToPointTypes.push_back(identInfo_MPI_Irecv);
    mpiNonBlockingTypes.push_back(identInfo_MPI_Irecv);
    assert(identInfo_MPI_Irecv);

    identInfo_MPI_Ssend = &context.Idents.get("MPI_Ssend");
    mpiSendTypes.push_back(identInfo_MPI_Ssend);
    mpiPointToPointTypes.push_back(identInfo_MPI_Ssend);
    mpiBlockingTypes.push_back(identInfo_MPI_Ssend);
    assert(identInfo_MPI_Ssend);

    identInfo_MPI_Issend = &context.Idents.get("MPI_Issend");
    mpiSendTypes.push_back(identInfo_MPI_Issend);
    mpiPointToPointTypes.push_back(identInfo_MPI_Issend);
    mpiNonBlockingTypes.push_back(identInfo_MPI_Issend);
    assert(identInfo_MPI_Issend);

    identInfo_MPI_Bsend = &context.Idents.get("MPI_Bsend");
    mpiSendTypes.push_back(identInfo_MPI_Bsend);
    mpiPointToPointTypes.push_back(identInfo_MPI_Bsend);
    mpiBlockingTypes.push_back(identInfo_MPI_Bsend);
    assert(identInfo_MPI_Bsend);

    // validate
    identInfo_MPI_Rsend = &context.Idents.get("MPI_Rsend");
    mpiSendTypes.push_back(identInfo_MPI_Rsend);
    mpiPointToPointTypes.push_back(identInfo_MPI_Rsend);
    mpiBlockingTypes.push_back(identInfo_MPI_Rsend);
    assert(identInfo_MPI_Rsend);

    // non communicating functions
    identInfo_MPI_Comm_rank = &context.Idents.get("MPI_Comm_rank");
    assert(identInfo_MPI_Comm_rank);
}

// classification functions–––––––––––––––––––––––––––––––––––––––––––––––––
//
/**
 * Check if MPI send function
 */
bool MPI_ASTVisitor::isSendType(const IdentifierInfo *identInfo) const {
    return lx::cont::isContained(mpiSendTypes, identInfo);
}

/**
 * Check if MPI recv function
 */
bool MPI_ASTVisitor::isRecvType(const IdentifierInfo *identInfo) const {
    return lx::cont::isContained(mpiRecvTypes, identInfo);
}

/**
 * Check if MPI blocking function
 */
bool MPI_ASTVisitor::isBlockingType(const IdentifierInfo *identInfo) const {
    return lx::cont::isContained(mpiBlockingTypes, identInfo);
}

/**
 * Check if MPI nonblocking function
 */
bool MPI_ASTVisitor::isNonBlockingType(const IdentifierInfo *identInfo) const {
    return lx::cont::isContained(mpiNonBlockingTypes, identInfo);
}

/**
 * Check if MPI point to point function
 */
bool MPI_ASTVisitor::isPointToPointType(const IdentifierInfo *identInfo) const {
    return lx::cont::isContained(mpiPointToPointTypes, identInfo);
}

/**
 * Check if MPI point to collective function
 */
bool MPI_ASTVisitor::isPointToCollType(const IdentifierInfo *identInfo) const {
    return lx::cont::isContained(mpiPointToCollTypes, identInfo);
}

/**
 * Check if MPI collective to point function
 */
bool MPI_ASTVisitor::isCollToPointType(const IdentifierInfo *identInfo) const {
    return lx::cont::isContained(mpiCollToPointTypes, identInfo);
}

/**
 * Check if MPI collective to collective function
 */
bool MPI_ASTVisitor::isCollToCollType(const IdentifierInfo *identInfo) const {
    return lx::cont::isContained(mpiCollToCollTypes, identInfo);
}

// visitor functions––––––––––––––––––––––––––––––––––––––––––––––––––––––

bool MPI_ASTVisitor::VisitDecl(Decl *declaration) {
    // std::cout << declaration->getDeclKindName() << std::endl;
    return true;
}

// MPI_ASTVisitor::Visits all function definitions
// (schema in the scope of one function can be evaluated easily)
bool MPI_ASTVisitor::VisitFunctionDecl(FunctionDecl *functionDecl) {
    return true;
}

bool MPI_ASTVisitor::VisitDeclRefExpr(DeclRefExpr *expression) {
    // TODO this works ok
    if (expression->getDecl()->getIdentifier() == identInfo_MPI_Send) {
        // expression->getDecl()->getObjCFStringFormattingFamily
    }
    return true;
}

/**
 * Called when function calls are executed.
 *
 * @param callExpr
 *
 * @return
 */
bool MPI_ASTVisitor::VisitCallExpr(CallExpr *callExpr) {
    const FunctionDecl *functionDecl = callExpr->getDirectCallee();

    if (duplicateMPICall(callExpr)) {
        std::cout << "dupli" << std::endl;
    }

    if (isRecvType(functionDecl->getIdentifier())) {
        std::cout << "recv type" << std::endl;
        mpiCalls.push_back(callExpr);
    }

    if (isSendType(functionDecl->getIdentifier())) {
        std::cout << "send type" << std::endl;
        mpiCalls.push_back(callExpr);
    }

    return true;
}

const Type *MPI_ASTVisitor::getType(const DeclRefExpr *decl) const {
    if (auto var = dyn_cast<ValueDecl>(decl->getDecl())) {
        if (var->getType()->isPointerType()) {
            return var->getType()->getPointeeType().getTypePtr();
        } else {
            bool isConst =
                var->getType().isConstant(analysisDeclContext_.getASTContext());
            std::cout << isConst << std::endl;
            return var->getType().getTypePtr();
        }
    }
    return nullptr;
}

const Type *MPI_ASTVisitor::getType(const CallExpr *callExpr,
                                    size_t idx) const {
    if (auto castExpr = dyn_cast<ImplicitCastExpr>(callExpr->getArg(idx))) {
        if (auto uno = dyn_cast<UnaryOperator>(castExpr->getSubExpr())) {
            if (auto decl = dyn_cast<DeclRefExpr>(uno->getSubExpr())) {
                return getType(decl);
            }
        } else if (auto imp =
                       dyn_cast<ImplicitCastExpr>(castExpr->getSubExpr())) {
            if (auto decl = dyn_cast<DeclRefExpr>(imp->getSubExpr())) {
                return getType(decl);
            }
        }
    }
    return nullptr;
}

/// A simple visitor to record what VarDecls occur in EH-handling code.
class FindVarDecl : public RecursiveASTVisitor<FindVarDecl> {
public:
    // TODO how to save argument list

    bool VisitDeclRefExpr(DeclRefExpr *DR) {
        if (const VarDecl *D = dyn_cast<VarDecl>(DR->getDecl())) {
            D->dumpColor();
        }
        return true;
    }

    bool VisitBinaryOperator(clang::BinaryOperator *binOp) {
        // TODO needs to be saved
        binOp->getOpcode();
        binOp->dumpColor();
        return true;
    }

    bool VisitIntegerLiteral(clang::IntegerLiteral *intLiteral) {
        intLiteral->dumpColor();
        return true;
    }
};

// TODO
// argumente können wie folgt strukturiert sein

/**
 * Check if the exact same call is already in the MPI function call set.
 * (ty = type)
 *
 * @param callEvent
 * @param mpiFnCallSet set searched for identical calls
 *
 * @return is equal call in list
 */
bool MPI_ASTVisitor::duplicateMPICall(CallExpr *callExpr) const {
    bool identical{false};

    if (isSendType(callExpr->getDirectCallee()->getIdentifier())) {
        // callExpr->dumpColor();
        // auto t = getType(callExpr, 0);
        // std::cout << t << std::endl;
    }

    // check for identical call
    for (const CallExpr *executedCall : mpiCalls) {
        // if calls have the same identifier - implies same number of args
        if (executedCall->getDirectCallee()->getIdentifier() ==
            callExpr->getDirectCallee()->getIdentifier()) {
            // callExpr->dumpColor();
            // identical = true;
            std::cout << "entered dupli" << std::endl;
            // const size_t numArgs{callExpr->getNumArgs()};
            std::cout << "comp" << std::endl;
            // TODO implement comparison

            FindVarDecl finder{};
            // TODO dont use for loop just put values found in container

            finder.TraverseCallExpr(callExpr);
            // TODO

            // for (size_t i = 0; i < numArgs; ++i) {

                // not recursive (visitor function)
                // finder.VisitExpr(callExpr->getArg(i));


                // break;
                // if (callExpr->getArg(i) != executedCall->getArg(i)) {
                // std::cout << "not" << i << std::endl;
                // // callExpr->getArg(i)->dumpColor();
                // // executedCall->getArg(i)->dumpColor();
                // // call not identical, check next
                // identical = false;
                // break;
                // }
            // }
            // end if identical call was found
            if (identical) break;
        }
    }
    return identical;
}

class MPISchemaCheckerAST
    : public Checker<check::ASTCodeBody, check::EndOfTranslationUnit,
                     check::EndAnalysis> {
public:
    void checkASTCodeBody(const Decl *decl, AnalysisManager &analysisManager,
                          BugReporter &bugReporter) const {
        MPI_ASTVisitor visitor{bugReporter, *this,
                               *analysisManager.getAnalysisDeclContext(decl)};
        visitor.TraverseDecl(const_cast<Decl *>(decl));
    }

    // TODO save to rely on this?
    void checkEndOfTranslationUnit(const TranslationUnitDecl *TU,
                                   AnalysisManager &mgr,
                                   BugReporter &BR) const {
        // why does llvm has special out function?
        // llvm::outs() << "end of translation unit" << '\n';
    }

    // TODO save to rely on this?
    void checkEndAnalysis(ExplodedGraph &G, BugReporter &B,
                          ExprEngine &Eng) const {
        static bool finalAnalysis{false};
        if (!finalAnalysis) {
            finalAnalysis = !finalAnalysis;
            // llvm::outs() << "end of analysis" << '\n';
        }
    }
};

void ento::registerMPISchemaCheckerAST(CheckerManager &mgr) {
    mgr.registerChecker<MPISchemaCheckerAST>();
}
