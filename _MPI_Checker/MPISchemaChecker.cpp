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

const std::string bugTypeEfficiency{"schema efficiency"};
const std::string bugTypeArgumentType{"argument type"};

struct MPICall {
public:
    MPICall(CallExpr *callExpr,
            llvm::SmallVector<mpi::SingleArgVisitor, 8> &&arguments)
        : callExpr_{callExpr}, arguments_{std::move(arguments)} {
        const FunctionDecl *functionDeclNew = callExpr_->getDirectCallee();
        identInfo_ = functionDeclNew->getIdentifier();
    };
    CallExpr *callExpr_;
    llvm::SmallVector<SingleArgVisitor, 8> arguments_;
    IdentifierInfo *identInfo_;
    unsigned long id_{id++};

    static llvm::SmallVector<MPICall, 16> visitedCalls;

private:
    static unsigned long id;
};
llvm::SmallVector<MPICall, 16> MPICall::visitedCalls;
unsigned long MPICall::id{0};

/**
 * Initializes function identifiers. Instead of using strings,
 * indentifier-pointers are initially captured
 * to recognize functions during analysis by comparison later.
 *
 * @param current ast-context used for analysis
 */
void MPIFunctionClassifier::identifierInit(
    clang::ento::AnalysisManager &analysisManager) {
    ASTContext &context = analysisManager.getASTContext();

    // init function identifiers
    // and copy them into the correct classification containers
    identInfo_MPI_Send_ = &context.Idents.get("MPI_Send");
    mpiSendTypes_.push_back(identInfo_MPI_Send_);
    mpiPointToPointTypes_.push_back(identInfo_MPI_Send_);
    mpiBlockingTypes_.push_back(identInfo_MPI_Send_);
    mpiType_.push_back(identInfo_MPI_Send_);
    assert(identInfo_MPI_Send_);

    identInfo_MPI_Recv_ = &context.Idents.get("MPI_Recv");
    mpiRecvTypes_.push_back(identInfo_MPI_Recv_);
    mpiPointToPointTypes_.push_back(identInfo_MPI_Recv_);
    mpiBlockingTypes_.push_back(identInfo_MPI_Recv_);
    mpiType_.push_back(identInfo_MPI_Recv_);
    assert(identInfo_MPI_Recv_);

    identInfo_MPI_Isend_ = &context.Idents.get("MPI_Isend");
    mpiSendTypes_.push_back(identInfo_MPI_Isend_);
    mpiPointToPointTypes_.push_back(identInfo_MPI_Isend_);
    mpiNonBlockingTypes_.push_back(identInfo_MPI_Isend_);
    mpiType_.push_back(identInfo_MPI_Isend_);
    assert(identInfo_MPI_Isend_);

    identInfo_MPI_Irecv_ = &context.Idents.get("MPI_Irecv");
    mpiRecvTypes_.push_back(identInfo_MPI_Irecv_);
    mpiPointToPointTypes_.push_back(identInfo_MPI_Irecv_);
    mpiNonBlockingTypes_.push_back(identInfo_MPI_Irecv_);
    mpiType_.push_back(identInfo_MPI_Irecv_);
    assert(identInfo_MPI_Irecv_);

    identInfo_MPI_Ssend_ = &context.Idents.get("MPI_Ssend");
    mpiSendTypes_.push_back(identInfo_MPI_Ssend_);
    mpiPointToPointTypes_.push_back(identInfo_MPI_Ssend_);
    mpiBlockingTypes_.push_back(identInfo_MPI_Ssend_);
    mpiType_.push_back(identInfo_MPI_Ssend_);
    assert(identInfo_MPI_Ssend_);

    identInfo_MPI_Issend_ = &context.Idents.get("MPI_Issend");
    mpiSendTypes_.push_back(identInfo_MPI_Issend_);
    mpiPointToPointTypes_.push_back(identInfo_MPI_Issend_);
    mpiNonBlockingTypes_.push_back(identInfo_MPI_Issend_);
    mpiType_.push_back(identInfo_MPI_Issend_);
    assert(identInfo_MPI_Issend_);

    identInfo_MPI_Bsend_ = &context.Idents.get("MPI_Bsend");
    mpiSendTypes_.push_back(identInfo_MPI_Bsend_);
    mpiPointToPointTypes_.push_back(identInfo_MPI_Bsend_);
    mpiBlockingTypes_.push_back(identInfo_MPI_Bsend_);
    mpiType_.push_back(identInfo_MPI_Bsend_);
    assert(identInfo_MPI_Bsend_);

    // validate
    identInfo_MPI_Rsend_ = &context.Idents.get("MPI_Rsend");
    mpiSendTypes_.push_back(identInfo_MPI_Rsend_);
    mpiPointToPointTypes_.push_back(identInfo_MPI_Rsend_);
    mpiBlockingTypes_.push_back(identInfo_MPI_Rsend_);
    mpiType_.push_back(identInfo_MPI_Rsend_);
    assert(identInfo_MPI_Rsend_);

    // non communicating functions
    identInfo_MPI_Comm_rank_ = &context.Idents.get("MPI_Comm_rank");
    mpiType_.push_back(identInfo_MPI_Comm_rank_);
    assert(identInfo_MPI_Comm_rank_);
}

// classification functions–––––––––––––––––––––––––––––––––––––––––––––––––

/**
 * Check if MPI send function
 */
bool MPIFunctionClassifier::isMPIType(const IdentifierInfo *identInfo) const {
    return cont::isContained(mpiType_, identInfo);
}

/**
 * Check if MPI send function
 */
bool MPIFunctionClassifier::isSendType(const IdentifierInfo *identInfo) const {
    return cont::isContained(mpiSendTypes_, identInfo);
}

/**
 * Check if MPI recv function
 */
bool MPIFunctionClassifier::isRecvType(const IdentifierInfo *identInfo) const {
    return cont::isContained(mpiRecvTypes_, identInfo);
}

/**
 * Check if MPI blocking function
 */
bool MPIFunctionClassifier::isBlockingType(
    const IdentifierInfo *identInfo) const {
    return cont::isContained(mpiBlockingTypes_, identInfo);
}

/**
 * Check if MPI nonblocking function
 */
bool MPIFunctionClassifier::isNonBlockingType(
    const IdentifierInfo *identInfo) const {
    return cont::isContained(mpiNonBlockingTypes_, identInfo);
}

/**
 * Check if MPI point to point function
 */
bool MPIFunctionClassifier::isPointToPointType(
    const IdentifierInfo *identInfo) const {
    return cont::isContained(mpiPointToPointTypes_, identInfo);
}

/**
 * Check if MPI point to collective function
 */
bool MPIFunctionClassifier::isPointToCollType(
    const IdentifierInfo *identInfo) const {
    return cont::isContained(mpiPointToCollTypes_, identInfo);
}

/**
 * Check if MPI collective to point function
 */
bool MPIFunctionClassifier::isCollToPointType(
    const IdentifierInfo *identInfo) const {
    return cont::isContained(mpiCollToPointTypes_, identInfo);
}

/**
 * Check if MPI collective to collective function
 */
bool MPIFunctionClassifier::isCollToCollType(
    const IdentifierInfo *identInfo) const {
    return cont::isContained(mpiCollToCollTypes_, identInfo);
}

// visitor functions––––––––––––––––––––––––––––––––––––––––––––––––––––––

bool MPI_ASTVisitor::VisitDecl(Decl *declaration) {
    // std::cout << declaration->getDeclKindName() << std::endl;
    return true;
}

bool MPI_ASTVisitor::VisitFunctionDecl(FunctionDecl *functionDecl) {
    // to keep track which function implementation is currently analysed
    if (functionDecl->clang::Decl::hasBody() && !functionDecl->isInlined()) {
        // to make display of function in diagnostics available
        bugReporter_.currentFunctionDecl_ = functionDecl;
    }
    return true;
}

bool MPI_ASTVisitor::VisitDeclRefExpr(DeclRefExpr *expression) {
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
    if (funcClassifier_.isMPIType(functionDecl->getIdentifier())) {
        // build argument vector
        llvm::SmallVector<SingleArgVisitor, 8> arguments;
        for (size_t i = 0; i < callExpr->getNumArgs(); ++i) {
            // triggers SingleArgVisitor ctor -> traversal
            arguments.emplace_back(callExpr, i);
        }

        MPICall mpiCall{callExpr, std::move(arguments)};
        checkForFloatArgs(mpiCall);

        MPICall::visitedCalls.push_back(std::move(mpiCall));
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

void MPI_ASTVisitor::checkForFloatArgs(const MPICall &mpiCall) const {
    if (funcClassifier_.isPointToPointType(mpiCall.identInfo_)) {
        auto indicesToCheck = {MPIPointToPoint::kCount, MPIPointToPoint::kRank,
                               MPIPointToPoint::kTag};

        // iterate indices which should not have float arguments
        for (size_t idx : indicesToCheck) {
            // check for float variables
            auto &arg = mpiCall.arguments_[idx];
            auto &vars = arg.vars_;
            for (auto &var : vars) {
                if (var->getType()->isFloatingType()) {
                    bugReporter_.reportFloat(mpiCall.callExpr_, idx,
                                             FloatArgType::kVariable);
                }
            }
            // check for float literals
            if (arg.floatingLiterals_.size()) {
                bugReporter_.reportFloat(mpiCall.callExpr_, idx,
                                         FloatArgType::kLiteral);
            }

            // check for float return values from functions
            auto &functions = arg.functions_;
            for (auto &function : functions) {
                if (function->getReturnType()->isFloatingType()) {
                    bugReporter_.reportFloat(mpiCall.callExpr_, idx,
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
bool MPI_ASTVisitor::fullArgumentComparison(const MPICall &callOne,
                                            const MPICall &callTwo,
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

void MPI_ASTVisitor::checkForDuplicatePointToPoint(
    const MPICall &callToCheck) const {
    for (const MPICall &comparedCall : MPICall::visitedCalls) {
        if (!funcClassifier_.isPointToPointType(comparedCall.identInfo_))
            continue;
        // do not check against the call itself
        if (callToCheck.id_ == comparedCall.id_) continue;
        // both must be of send or receive type
        if (funcClassifier_.isSendType(callToCheck.identInfo_) !=
            funcClassifier_.isSendType(comparedCall.identInfo_))
            continue;

        // compare buffer (types) ––––––––––––––––––––––––––––––––––––––

        // auto bufferNew =
        // callToCheck.arguments_[MPIPointToPoint::kBuf].vars_.front();
        // auto bufferPrev =
        // comparedCall.arguments_[MPIPointToPoint::kBuf].vars_.front();
        // if (bufferNew != bufferPrev) continue;

        // if (getBuiltinType(bufferTypeNew) !=
        // getBuiltinType(bufferTypePrev))
        // continue;

        // argument types which are compared by all 'components' –––––––
        bool identical = true;
        auto indicesToCheck = {MPIPointToPoint::kCount, MPIPointToPoint::kRank,
                               MPIPointToPoint::kTag};
        for (size_t idx : indicesToCheck) {
            if (!fullArgumentComparison(callToCheck, comparedCall, idx)) {
                identical = false;
                break;  // end inner loop
            }
        }
        if (!identical) continue;

        // compare specified mpi datatypes –––––––––––––––––––––––––––––
        auto mpiTypeNew =
            callToCheck.arguments_[MPIPointToPoint::kDatatype].vars_.front();
        auto mpiTypePrev =
            comparedCall.arguments_[MPIPointToPoint::kDatatype].vars_.front();

        if (mpiTypeNew->getName() != mpiTypePrev->getName()) {
            continue;
        }

        // if function reaches this point
        // all arguments have been equal
        bugReporter_.reportDuplicate(comparedCall.callExpr_,
                                     callToCheck.callExpr_);
        // end loop
        break;
    }
}

/**
 * Check if the exact same call was already executed.
 *
 * @param callEvent
 * @param mpiFnCallSet set searched for identical calls
 *
 * @return is equal call in list
 */
void MPI_ASTVisitor::checkForDuplicates() const {
    for (const MPICall &mpiCall : MPICall::visitedCalls) {
        if (funcClassifier_.isPointToPointType(mpiCall.identInfo_)) {
            checkForDuplicatePointToPoint(mpiCall);
        }
    }
}

// bug reports–––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
void MPIBugReporter::reportFloat(CallExpr *callExpr, size_t idx,
                                 FloatArgType type) const {
    auto d = analysisManager_.getAnalysisDeclContext(currentFunctionDecl_);
    PathDiagnosticLocation location = PathDiagnosticLocation::createBegin(
        callExpr, bugReporter_.getSourceManager(), d);

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
        d->getDecl(), &checkerBase_, bugTypeArgumentType, bugGroupMPIError,
        "float " + typeAsString + " used at index: " + indexAsString, location,
        range);
}

void MPIBugReporter::reportDuplicate(const CallExpr *matchedCall,
                                     const CallExpr *duplicateCall) const {
    auto analysisDeclCtx =
        analysisManager_.getAnalysisDeclContext(currentFunctionDecl_);

    PathDiagnosticLocation location = PathDiagnosticLocation::createBegin(
        duplicateCall, bugReporter_.getSourceManager(), analysisDeclCtx);

    std::string lineNo =
        matchedCall->getCallee()->getSourceRange().getBegin().printToString(
            bugReporter_.getSourceManager());

    // split written string into parts
    std::vector<std::string> strs = util::split(lineNo, ':');
    lineNo = strs.at(strs.size() - 2);

    SourceRange range = duplicateCall->getCallee()->getSourceRange();

    bugReporter_.EmitBasicReport(
        analysisDeclCtx->getDecl(), &checkerBase_, bugTypeEfficiency,
        bugGroupMPIWarning,
        "identical communication "
        "arguments (count, mpi-datatype, rank, tag) used in " +
            matchedCall->getDirectCallee()->getNameAsString() + " in line: " +
            lineNo + " \n\nconsider to summarize these calls",
        location, range);
}

/**
 * Main checker host class. Receives callback for every translation unit.
 * Class name determines checker name to specify when the command line
 * is invoked for static analysis.
 */
class MPISchemaChecker : public Checker<check::ASTDecl<TranslationUnitDecl>> {
private:
public:
    void checkASTDecl(const TranslationUnitDecl *tuDecl,
                      AnalysisManager &analysisManager,
                      BugReporter &bugReporter) const {
        MPI_ASTVisitor visitor{bugReporter, *this, analysisManager};
        visitor.TraverseTranslationUnitDecl(
            const_cast<TranslationUnitDecl *>(tuDecl));

        // checks invoked after travering a translation unit
        visitor.checkForDuplicates();
    }
};

}  // end of namespace: mpi

void ento::registerMPISchemaChecker(CheckerManager &mgr) {
    mgr.registerChecker<mpi::MPISchemaChecker>();
}
