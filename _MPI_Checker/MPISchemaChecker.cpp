#include <utility>
#include "llvm/ADT/SmallVector.h"

#include "MPISchemaChecker.hpp"
#include "Container.hpp"
#include "Utility.hpp"

using namespace clang;
using namespace ento;

namespace mpi {

const std::string bugGroupMPIError{"MPI Error"};
const std::string bugGroupMPIWarning{"MPI Warning"};

struct MPICall {
    MPICall(CallExpr *callExpr,
            llvm::SmallVector<mpi::SingleArgVisitor, 8> &&arguments)
        : callExpr_{callExpr}, arguments_{std::move(arguments)} {
        const FunctionDecl *functionDeclNew = callExpr_->getDirectCallee();
        identInfo_ = functionDeclNew->getIdentifier();
    };
    CallExpr *callExpr_;
    llvm::SmallVector<SingleArgVisitor, 8> arguments_;
    IdentifierInfo *identInfo_;
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
    return cont::isContained(mpiType, identInfo);
}

/**
 * Check if MPI send function
 */
bool MPI_ASTVisitor::isSendType(const IdentifierInfo *identInfo) const {
    return cont::isContained(mpiSendTypes, identInfo);
}

/**
 * Check if MPI recv function
 */
bool MPI_ASTVisitor::isRecvType(const IdentifierInfo *identInfo) const {
    return cont::isContained(mpiRecvTypes, identInfo);
}

/**
 * Check if MPI blocking function
 */
bool MPI_ASTVisitor::isBlockingType(const IdentifierInfo *identInfo) const {
    return cont::isContained(mpiBlockingTypes, identInfo);
}

/**
 * Check if MPI nonblocking function
 */
bool MPI_ASTVisitor::isNonBlockingType(const IdentifierInfo *identInfo) const {
    return cont::isContained(mpiNonBlockingTypes, identInfo);
}

/**
 * Check if MPI point to point function
 */
bool MPI_ASTVisitor::isPointToPointType(const IdentifierInfo *identInfo) const {
    return cont::isContained(mpiPointToPointTypes, identInfo);
}

/**
 * Check if MPI point to collective function
 */
bool MPI_ASTVisitor::isPointToCollType(const IdentifierInfo *identInfo) const {
    return cont::isContained(mpiPointToCollTypes, identInfo);
}

/**
 * Check if MPI collective to point function
 */
bool MPI_ASTVisitor::isCollToPointType(const IdentifierInfo *identInfo) const {
    return cont::isContained(mpiCollToPointTypes, identInfo);
}

/**
 * Check if MPI collective to collective function
 */
bool MPI_ASTVisitor::isCollToCollType(const IdentifierInfo *identInfo) const {
    return cont::isContained(mpiCollToCollTypes, identInfo);
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
        // build argument vector
        llvm::SmallVector<SingleArgVisitor, 8> arguments;
        for (size_t i = 0; i < callExpr->getNumArgs(); ++i) {
            // triggers SingleArgVisitor traversal
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
 * Ex.: const int, int* -> int (both have builtin type 'int')
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

        // iterate indices which should not have float arguments
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
                    reportFloat(mpiCall.callExpr_, idx,
                                FloatArgType::kReturnType);
                }
            }
        }
    }
}

/**
 * Compares all components of two arguments for equality
 * obtained from given calls with index.
 *
 * @param callOne
 * @param callTwo
 * @param idx
 *
 * @return areEqual
 */
bool MPI_ASTVisitor::fullArgumentComparison(MPICall &callOne, MPICall &callTwo,
                                            size_t idx) const {
    auto argOne = callOne.arguments_[idx];
    auto argTwo = callTwo.arguments_[idx];

    // operators
    if (!util::isPermutation(argOne.binaryOperators_, argTwo.binaryOperators_))
        return false;

    // variables
    if (!util::isPermutation(argOne.vars_, argTwo.vars_)) return false;

    // int literals
    if (!util::isPermutation(argOne.integerLiterals_, argTwo.integerLiterals_))
        return false;

    // float literals
    // just compare count, floats should not be compared by value
    // https://tinyurl.com/ks8smw4
    if (argOne.floatingLiterals_.size() != argTwo.floatingLiterals_.size()) {
        return false;
    }

    // functions
    if (!util::isPermutation(argOne.functions_, argTwo.functions_))
        return false;

    return true;
}

// TODO report datatype missmatch
// between buffer and mpi datatype

/**
 * Check if the exact same call was already executed.
 *
 * @param callEvent
 * @param mpiFnCallSet set searched for identical calls
 *
 * @return is equal call in list
 */
void MPI_ASTVisitor::checkForDuplicate(MPICall &newCall) const {
    if (isPointToPointType(newCall.identInfo_)) {
        for (MPICall &prevCall : mpiCalls) {
            // compare function identifiers –––––––––––––––––––––––––––––––––
            if (newCall.identInfo_ != prevCall.identInfo_) continue;

            // compare buffer (types) ––––––––––––––––––––––––––––––––––––––
            auto bufferTypeNew =
                newCall.arguments_[MPIPointToPoint::kBuf].vars_.front();
            auto bufferTypePrev =
                prevCall.arguments_[MPIPointToPoint::kBuf].vars_.front();

            if (getBuiltinType(bufferTypeNew) != getBuiltinType(bufferTypePrev))
                continue;

            // argument types which are compared by all 'components' –––––––
            bool identical = true;
            auto indicesToCheck = {MPIPointToPoint::kCount,
                                   MPIPointToPoint::kRank,
                                   MPIPointToPoint::kTag};
            for (size_t idx : indicesToCheck) {
                if (!fullArgumentComparison(newCall, prevCall, idx)) {
                    identical = false;
                    break;  // end inner loop
                }
            }
            if (!identical) continue;

            // compare specified mpi datatypes –––––––––––––––––––––––––––––
            auto mpiTypeNew =
                newCall.arguments_[MPIPointToPoint::kDatatype].vars_.front();
            auto mpiTypePrev =
                prevCall.arguments_[MPIPointToPoint::kDatatype].vars_.front();

            if (mpiTypeNew->getName() != mpiTypePrev->getName()) {
                continue;
            }

            // if function reaches this point
            // all arguments have been equal
            reportDuplicate(prevCall.callExpr_, newCall.callExpr_);
            // end loop
            break;
        }
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
        bugGroupMPIError,
        "float " + typeAsString + " used at index: " + indexAsString, location,
        range);
}

void MPI_ASTVisitor::reportDuplicate(CallExpr *matchedCall,
                                     CallExpr *duplicateCall) const {
    PathDiagnosticLocation location = PathDiagnosticLocation::createBegin(
        duplicateCall, bugReporter_.getSourceManager(), &analysisDeclContext_);

    std::string lineNo =
        matchedCall->getCallee()->getSourceRange().getBegin().printToString(
            bugReporter_.getSourceManager());

    // split written strings into parts
    std::vector<std::string> strs = util::split(lineNo, ':');
    lineNo = strs.at(strs.size() - 2);

    SourceRange range = duplicateCall->getCallee()->getSourceRange();

    bugReporter_.EmitBasicReport(
        analysisDeclContext_.getDecl(), &checkerBase_, "duplicate",
        bugGroupMPIError, "exact duplicate of mpi call in line: " + lineNo,
        location, range);
}

class MPISchemaChecker
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

}  // end of namespace: mpi

void ento::registerMPISchemaChecker(CheckerManager &mgr) {
    mgr.registerChecker<mpi::MPISchemaChecker>();
}
