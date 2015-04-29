#include <utility>
#include "llvm/ADT/SmallVector.h"

#include "MPISchemaCheckerAST.hpp"
#include "ASTFinder.hpp"
#include "Container.hpp"

using namespace clang;
using namespace ento;

const std::string mpiBugGroupError{"MPI Error"};
const std::string mpiBugGroupWarning{"MPI Warning"};

/// A simple visitor to record what VarDecls occur in EH-handling code.
class SingleArgVisitor : public clang::RecursiveASTVisitor<SingleArgVisitor> {
public:
    SingleArgVisitor(CallExpr *argExpression, size_t idx) {
        TraverseStmt(argExpression->getArg(idx));
    }
    bool VisitDeclRefExpr(clang::DeclRefExpr *potentialVar) {
        if (clang::VarDecl *var =
                clang::dyn_cast<clang::VarDecl>(potentialVar->getDecl())) {
            vars_.push_back(var);
        } else if (clang::FunctionDecl *fn =
                       clang::dyn_cast<clang::FunctionDecl>(
                           potentialVar->getDecl())) {
            functions_.push_back(fn);
        }
        return true;
    }

    bool VisitBinaryOperator(clang::BinaryOperator *op) {
        binaryOperators_.push_back(op);
        return true;
    }

    bool VisitIntegerLiteral(IntegerLiteral *intLiteral) {
        integerLiterals_.push_back(intLiteral);
        return true;
    }

    bool VisitFloatingLiteral(FloatingLiteral *floatLiteral) {
        floatingLiterals_.push_back(floatLiteral);
        return true;
    }

    llvm::SmallVector<BinaryOperator *, 2> binaryOperators_{};
    llvm::SmallVector<VarDecl *, 2> vars_{};
    llvm::SmallVector<FunctionDecl *, 2> functions_{};
    llvm::SmallVector<IntegerLiteral *, 2> integerLiterals_{};
    llvm::SmallVector<FloatingLiteral *, 2> floatingLiterals_{};
};

struct MPICall {
    MPICall(CallExpr *callExpr,
            llvm::SmallVector<SingleArgVisitor, 8> &&arguments)
        : callExpr_{callExpr}, arguments_{arguments} {};
    CallExpr *callExpr_;
    llvm::SmallVector<SingleArgVisitor, 8> arguments_;
};
llvm::SmallVector<MPICall, 16> mpiCalls;

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
    mpiType.push_back(identInfo_MPI_Send);
    assert(identInfo_MPI_Send);

    identInfo_MPI_Recv = &context.Idents.get("MPI_Recv");
    mpiRecvTypes.push_back(identInfo_MPI_Recv);
    mpiPointToPointTypes.push_back(identInfo_MPI_Recv);
    mpiBlockingTypes.push_back(identInfo_MPI_Recv);
    mpiType.push_back(identInfo_MPI_Recv);
    assert(identInfo_MPI_Recv);

    identInfo_MPI_Isend = &context.Idents.get("MPI_Isend");
    mpiSendTypes.push_back(identInfo_MPI_Isend);
    mpiPointToPointTypes.push_back(identInfo_MPI_Isend);
    mpiNonBlockingTypes.push_back(identInfo_MPI_Isend);
    mpiType.push_back(identInfo_MPI_Isend);
    assert(identInfo_MPI_Isend);

    identInfo_MPI_Irecv = &context.Idents.get("MPI_Irecv");
    mpiRecvTypes.push_back(identInfo_MPI_Irecv);
    mpiPointToPointTypes.push_back(identInfo_MPI_Irecv);
    mpiNonBlockingTypes.push_back(identInfo_MPI_Irecv);
    mpiType.push_back(identInfo_MPI_Irecv);
    assert(identInfo_MPI_Irecv);

    identInfo_MPI_Ssend = &context.Idents.get("MPI_Ssend");
    mpiSendTypes.push_back(identInfo_MPI_Ssend);
    mpiPointToPointTypes.push_back(identInfo_MPI_Ssend);
    mpiBlockingTypes.push_back(identInfo_MPI_Ssend);
    mpiType.push_back(identInfo_MPI_Ssend);
    assert(identInfo_MPI_Ssend);

    identInfo_MPI_Issend = &context.Idents.get("MPI_Issend");
    mpiSendTypes.push_back(identInfo_MPI_Issend);
    mpiPointToPointTypes.push_back(identInfo_MPI_Issend);
    mpiNonBlockingTypes.push_back(identInfo_MPI_Issend);
    mpiType.push_back(identInfo_MPI_Issend);
    assert(identInfo_MPI_Issend);

    identInfo_MPI_Bsend = &context.Idents.get("MPI_Bsend");
    mpiSendTypes.push_back(identInfo_MPI_Bsend);
    mpiPointToPointTypes.push_back(identInfo_MPI_Bsend);
    mpiBlockingTypes.push_back(identInfo_MPI_Bsend);
    mpiType.push_back(identInfo_MPI_Bsend);
    assert(identInfo_MPI_Bsend);

    // validate
    identInfo_MPI_Rsend = &context.Idents.get("MPI_Rsend");
    mpiSendTypes.push_back(identInfo_MPI_Rsend);
    mpiPointToPointTypes.push_back(identInfo_MPI_Rsend);
    mpiBlockingTypes.push_back(identInfo_MPI_Rsend);
    mpiType.push_back(identInfo_MPI_Rsend);
    assert(identInfo_MPI_Rsend);

    // non communicating functions
    identInfo_MPI_Comm_rank = &context.Idents.get("MPI_Comm_rank");
    mpiType.push_back(identInfo_MPI_Comm_rank);
    assert(identInfo_MPI_Comm_rank);
}

// classification functions–––––––––––––––––––––––––––––––––––––––––––––––––

/**
 * Check if MPI send function
 */
bool MPI_ASTVisitor::isMPIType(const IdentifierInfo *identInfo) const {
    return lx::cont::isContained(mpiType, identInfo);
}

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

    // check if float literal is used in schema
    if (isMPIType(functionDecl->getIdentifier())) {
        llvm::SmallVector<SingleArgVisitor, 8> arguments;
        for (size_t i = 0; i < callExpr->getNumArgs(); ++i) {
            arguments.emplace_back(callExpr, i);
        }

        // TODO save to just store the pointer?
        MPICall mpiCall{callExpr, std::move(arguments)};
        checkForFloatArgs(mpiCall);
        checkForDuplicate(mpiCall);

        mpiCalls.push_back(std::move(mpiCall));
    }

    return true;
}

/**
 * Returns builtin type for variable. Removes pointer and qualifier attributes.
 * Ex.: const int, int * -> int (both have builtin type 'int')
 *
 * @param var
 *
 * @return value type
 */
const Type *MPI_ASTVisitor::getBuiltinType(const ValueDecl *var) const {
    if (var->getType()->isPointerType()) {
        return var->getType()->getPointeeType()->getUnqualifiedDesugaredType();
    } else {
        return var->getType()->getUnqualifiedDesugaredType();
    }
}

void MPI_ASTVisitor::checkForFloatArgs(MPICall &mpiCall) {
    const FunctionDecl *functionDecl = mpiCall.callExpr_->getDirectCallee();
    if (isPointToPointType(functionDecl->getIdentifier())) {
        auto indicesToCheck = {MPIPointToPoint::kCount, MPIPointToPoint::kRank,
                               MPIPointToPoint::kTag};

        for (size_t idx : indicesToCheck) {
            // check for float variables
            auto &arg = mpiCall.arguments_[idx];
            auto &vars = arg.vars_;
            for (auto &var : vars) {
                if (var->getType()->isFloatingType()) {
                    reportFloat(mpiCall.callExpr_, idx,
                                FloatArgType::kVariable);
                }
            }
            // check for float literals
            if (arg.floatingLiterals_.size()) {
                reportFloat(mpiCall.callExpr_, idx, FloatArgType::kLiteral);
            }

            // check for float return values from functions
            auto &functions = arg.functions_;
            for (auto &function : functions) {
                if (function->getReturnType()->isFloatingType()) {
                    llvm::outs() << functions.size() << "\n";
                    reportFloat(mpiCall.callExpr_, idx,
                                FloatArgType::kReturnType);
                }
            }
        }
    }
}

// TODO
// argumente können wie folgt strukturiert sein

bool MPI_ASTVisitor::areMPICallExprsEqual(CallExpr *callExpr1,
                                          CallExpr *callExpr2) const {
    // if calls have the same identifier - implies same number of args
    if (callExpr1->getDirectCallee()->getIdentifier() !=
        callExpr2->getDirectCallee()->getIdentifier()) {
        return false;
    }

    if (isPointToPointType(callExpr1->getDirectCallee()->getIdentifier())) {
    }
    return true;
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
void MPI_ASTVisitor::checkForDuplicate(MPICall &discoveredCall) const {
    const FunctionDecl *functionDecl =
        discoveredCall.callExpr_->getDirectCallee();

    if (isPointToPointType(functionDecl->getIdentifier())) {
        // bool identical{false};

        auto bufArg = discoveredCall.arguments_[MPIPointToPoint::kBuf];
        auto type = getBuiltinType(bufArg.vars_.front());
        type->dump();
    }
}

// bug reports–––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
void MPI_ASTVisitor::reportFloat(CallExpr *callExpr, size_t idx,
                                 FloatArgType type) const {
    PathDiagnosticLocation location = PathDiagnosticLocation::createBegin(
        callExpr, bugReporter_.getSourceManager(), &analysisDeclContext_);

    std::string indexAsString{std::to_string(idx)};
    SourceRange range = callExpr->getCallee()->getSourceRange();

    std::string typeAsString;
    switch (type) {
        case FloatArgType::kLiteral:
            typeAsString = "literal";
            break;

        case FloatArgType::kVariable:
            typeAsString = "variable";
            break;

        case FloatArgType::kReturnType:
            typeAsString = "return value from function";
            break;
    }

    bugReporter_.EmitBasicReport(
        analysisDeclContext_.getDecl(), &checkerBase_, "float schema argument",
        mpiBugGroupError,
        "float " + typeAsString + " used at index: " + indexAsString, location,
        range);
}

void MPI_ASTVisitor::reportDuplicate(CallExpr *callExpr) const {
    PathDiagnosticLocation location = PathDiagnosticLocation::createBegin(
        callExpr, bugReporter_.getSourceManager(), &analysisDeclContext_);

    SourceRange range = callExpr->getCallee()->getSourceRange();

    bugReporter_.EmitBasicReport(
        analysisDeclContext_.getDecl(), &checkerBase_, "duplicate",
        mpiBugGroupError,
        "exact duplicate mpi call, might be summarized in a single call",
        location, range);
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

    void checkEndOfTranslationUnit(const TranslationUnitDecl *TU,
                                   AnalysisManager &mgr,
                                   BugReporter &BR) const {}

    // TODO save to rely on this?
    void checkEndAnalysis(ExplodedGraph &G, BugReporter &B,
                          ExprEngine &Eng) const {
        static bool finalAnalysis{false};
        if (!finalAnalysis) {
            finalAnalysis = !finalAnalysis;
        }
    }
};

void ento::registerMPISchemaCheckerAST(CheckerManager &mgr) {
    mgr.registerChecker<MPISchemaCheckerAST>();
}
