#include <utility>
#include "llvm/ADT/SmallVector.h"
#include "clang/Lex/Lexer.h"

#include "MPISchemaChecker.hpp"
#include "Container.hpp"
#include "Utility.hpp"

using namespace clang;
using namespace ento;

namespace mpi {

const std::string bugGroupMPIError{"MPI Error"};
const std::string bugGroupMPIWarning{"MPI Warning"};

const std::string bugTypeEfficiency{"schema efficiency"};
const std::string bugTypeInvalidArgumentType{"invalid argument type"};
const std::string bugTypeArgumentTypeMismatch{"buffer type mismatch"};

struct MPICall {
public:
    MPICall(CallExpr *callExpr,
            llvm::SmallVector<vis::SingleArgVisitor, 8> &&arguments)
        : callExpr_{callExpr}, arguments_{std::move(arguments)} {
        const FunctionDecl *functionDeclNew = callExpr_->getDirectCallee();
        identInfo_ = functionDeclNew->getIdentifier();
    };
    CallExpr *callExpr_;
    llvm::SmallVector<vis::SingleArgVisitor, 8> arguments_;
    IdentifierInfo *identInfo_;
    unsigned long id_{id++};
    mutable bool isMarked_;

    static llvm::SmallVector<MPICall, 16> visitedCalls;

private:
    static unsigned long id;
};
llvm::SmallVector<MPICall, 16> MPICall::visitedCalls;
unsigned long MPICall::id{0};

// classification ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
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

// bug reports–––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

/**
 * Reports mismach between buffer type and mpi datatype.
 * @param callExpr
 */
void MPIBugReporter::reportTypeMismatch(CallExpr *callExpr) const {
    auto adc = analysisManager_.getAnalysisDeclContext(currentFunctionDecl_);
    PathDiagnosticLocation location = PathDiagnosticLocation::createBegin(
        callExpr, bugReporter_.getSourceManager(), adc);

    SourceRange range = callExpr->getCallee()->getSourceRange();

    bugReporter_.EmitBasicReport(
        adc->getDecl(), &checkerBase_, bugTypeArgumentTypeMismatch,
        bugGroupMPIError, "buffer type and specified mpi type do not match",
        location, range);
}

/**
 * Report non-integer value usage at indices where not allowed.
 *
 * @param callExpr
 * @param idx
 * @param type
 */
void MPIBugReporter::reportInvalidArgumentType(CallExpr *callExpr, size_t idx,
                                               InvalidArgType type) const {
    auto d = analysisManager_.getAnalysisDeclContext(currentFunctionDecl_);
    PathDiagnosticLocation location = PathDiagnosticLocation::createBegin(
        callExpr, bugReporter_.getSourceManager(), d);

    std::string indexAsString{std::to_string(idx)};
    SourceRange range = callExpr->getCallee()->getSourceRange();

    std::string typeAsString;
    switch (type) {
        case InvalidArgType::kLiteral:
            typeAsString = "literal";
            break;

        case InvalidArgType::kVariable:
            typeAsString = "variable";
            break;

        case InvalidArgType::kReturnType:
            typeAsString = "return value from function";
            break;
    }

    bugReporter_.EmitBasicReport(
        d->getDecl(), &checkerBase_, bugTypeInvalidArgumentType,
        bugGroupMPIError,
        typeAsString + " used at index " + indexAsString + " is not valid",
        location, range);
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

// visitor –––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

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

bool MPI_ASTVisitor::VisitDeclRefExpr(DeclRefExpr *expression) { return true; }

bool MPI_ASTVisitor::VisitIfStmt(IfStmt *ifStmt) { return true; }

/**
 * Visited when function calls to execute are visited.
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
        llvm::SmallVector<vis::SingleArgVisitor, 8> arguments;
        for (size_t i = 0; i < callExpr->getNumArgs(); ++i) {
            // triggers SingleArgVisitor ctor -> traversal
            arguments.emplace_back(callExpr, i);
        }

        MPICall mpiCall{callExpr, std::move(arguments)};
        checkBufferTypeMatch(mpiCall);
        checkForInvalidArgs(mpiCall);

        MPICall::visitedCalls.push_back(std::move(mpiCall));
    }

    return true;
}

/**
 * Checks if buffer type and specified mpi datatype matches.
 *
 * @param mpiCall call to check type correspondence for
 */
void MPI_ASTVisitor::checkBufferTypeMatch(const MPICall &mpiCall) const {
    if (funcClassifier_.isPointToPointType(mpiCall.identInfo_)) {
        const VarDecl *bufferArg =
            mpiCall.arguments_[MPIPointToPoint::kBuf].vars_.front();

        // collect type information
        vis::TypeVisitor typeRetriever{vis::TypeVisitor(bufferArg->getType())};

        // get mpi datatype as string
        auto mpiDatatype = mpiCall.arguments_[MPIPointToPoint::kDatatype].expr_;
        StringRef mpiDatatypeString{util::sourceRangeAsStringRef(
            mpiDatatype->getSourceRange(), analysisManager_)};

        // check for exact width types (e.g. int16_t, uint32_t)
        if (typeRetriever.isTypedefType_) {
            checkExactWidthTypeMatch(mpiCall.callExpr_, typeRetriever,
                                     mpiDatatypeString);
            return;
        }

        // check for complex-floating types (e.g. float _Complex)
        if (typeRetriever.complexType_) {
            checkComplexTypeMatch(mpiCall.callExpr_, typeRetriever,
                                  mpiDatatypeString);
            return;
        }

        // check for basic builtin types (e.g. int, char)
        clang::BuiltinType *builtinTypeBuffer = typeRetriever.builtinType_;
        if (!builtinTypeBuffer) return;  // if no builtin type cancel checking

        if (builtinTypeBuffer->isCharType() ||
            builtinTypeBuffer->isWideCharType()) {
            checkCharTypeMatch(mpiCall.callExpr_, typeRetriever,
                               mpiDatatypeString);
        } else if (builtinTypeBuffer->isSignedInteger()) {
            checkSignedTypeMatch(mpiCall.callExpr_, typeRetriever,
                                 mpiDatatypeString);
        } else if (builtinTypeBuffer->isUnsignedIntegerType()) {
            checkUnsignedTypeMatch(mpiCall.callExpr_, typeRetriever,
                                   mpiDatatypeString);
        } else if (builtinTypeBuffer->isFloatingType()) {
            checkFloatTypeMatch(mpiCall.callExpr_, typeRetriever,
                                mpiDatatypeString);
        }
    }
}

void MPI_ASTVisitor::checkCharTypeMatch(CallExpr *callExpr,
                                        vis::TypeVisitor &visitor,
                                        llvm::StringRef mpiDatatype) const {
    bool isTypeMatching;
    switch (visitor.builtinType_->getKind()) {
        case BuiltinType::SChar:
            isTypeMatching =
                (mpiDatatype == "MPI_CHAR" || mpiDatatype == "MPI_SIGNED_CHAR");
            break;
        case BuiltinType::UChar:
            isTypeMatching = (mpiDatatype == "MPI_UNSIGNED_CHAR");
            break;
        case BuiltinType::WChar_S:
            isTypeMatching = (mpiDatatype == "MPI_WCHAR");
            break;
        case BuiltinType::WChar_U:
            isTypeMatching = (mpiDatatype == "MPI_UNSIGNED_CHAR");
            break;

        default:
            isTypeMatching = true;
    }

    if (!isTypeMatching) bugReporter_.reportTypeMismatch(callExpr);
}

void MPI_ASTVisitor::checkSignedTypeMatch(CallExpr *callExpr,
                                          vis::TypeVisitor &visitor,
                                          llvm::StringRef mpiDatatype) const {
    bool isTypeMatching;

    switch (visitor.builtinType_->getKind()) {
        case BuiltinType::Int:
            isTypeMatching = (mpiDatatype == "MPI_INT");
            break;
        case BuiltinType::Long:
            isTypeMatching = (mpiDatatype == "MPI_LONG");
            break;
        case BuiltinType::Short:
            isTypeMatching = (mpiDatatype == "MPI_SHORT");
            break;
        case BuiltinType::LongLong:
            isTypeMatching = (mpiDatatype == "MPI_LONG_LONG" ||
                              mpiDatatype == "MPI_LONG_LONG_INT");
            break;
        case BuiltinType::Bool:
            isTypeMatching = (mpiDatatype == "MPI_C_BOOL");
            break;
        default:
            isTypeMatching = true;
    }

    if (!isTypeMatching) bugReporter_.reportTypeMismatch(callExpr);
}

void MPI_ASTVisitor::checkUnsignedTypeMatch(CallExpr *callExpr,
                                            vis::TypeVisitor &visitor,
                                            llvm::StringRef mpiDatatype) const {
    bool isTypeMatching;

    switch (visitor.builtinType_->getKind()) {
        case BuiltinType::UInt:
            isTypeMatching = (mpiDatatype == "MPI_UNSIGNED");
            break;
        case BuiltinType::UShort:
            isTypeMatching = (mpiDatatype == "MPI_UNSIGNED_SHORT");
            break;
        case BuiltinType::ULong:
            isTypeMatching = (mpiDatatype == "MPI_UNSIGNED_LONG");
            break;
        case BuiltinType::ULongLong:
            isTypeMatching = (mpiDatatype == "MPI_UNSIGNED_LONG_LONG");
            break;

        default:
            isTypeMatching = true;
    }
    if (!isTypeMatching) bugReporter_.reportTypeMismatch(callExpr);
}

void MPI_ASTVisitor::checkFloatTypeMatch(CallExpr *callExpr,
                                         vis::TypeVisitor &visitor,
                                         llvm::StringRef mpiDatatype) const {
    bool isTypeMatching;

    switch (visitor.builtinType_->getKind()) {
        case BuiltinType::Float:
            isTypeMatching = (mpiDatatype == "MPI_FLOAT");
            break;
        case BuiltinType::Double:
            isTypeMatching = (mpiDatatype == "MPI_DOUBLE");
            break;
        case BuiltinType::LongDouble:
            isTypeMatching = (mpiDatatype == "MPI_LONG_DOUBLE");
            break;
        default:
            isTypeMatching = true;
    }
    if (!isTypeMatching) bugReporter_.reportTypeMismatch(callExpr);
}

void MPI_ASTVisitor::checkComplexTypeMatch(CallExpr *callExpr,
                                           vis::TypeVisitor &visitor,
                                           llvm::StringRef mpiDatatype) const {
    bool isTypeMatching;

    switch (visitor.builtinType_->getKind()) {
        case BuiltinType::Float:
            isTypeMatching = (mpiDatatype == "MPI_C_COMPLEX" ||
                              mpiDatatype == "MPI_C_FLOAT_COMPLEX");
            break;
        case BuiltinType::Double:
            isTypeMatching = (mpiDatatype == "MPI_C_DOUBLE_COMPLEX");
            break;
        case BuiltinType::LongDouble:
            isTypeMatching = (mpiDatatype == "MPI_C_LONG_DOUBLE_COMPLEX");
            break;
        default:
            isTypeMatching = true;
    }

    if (!isTypeMatching) bugReporter_.reportTypeMismatch(callExpr);
}

void MPI_ASTVisitor::checkExactWidthTypeMatch(
    CallExpr *callExpr, vis::TypeVisitor &visitor,
    llvm::StringRef mpiDatatype) const {
    // check typedef type match
    // no break needs to be specified for string switch
    bool isTypeMatching = llvm::StringSwitch<bool>(visitor.typedefTypeName_)
                              .Case("int8_t", (mpiDatatype == "MPI_INT8_T"))
                              .Case("int16_t", (mpiDatatype == "MPI_INT16_T"))
                              .Case("int32_t", (mpiDatatype == "MPI_INT32_T"))
                              .Case("int64_t", (mpiDatatype == "MPI_INT64_T"))

                              .Case("uint8_t", (mpiDatatype == "MPI_UINT8_T"))
                              .Case("uint16_t", (mpiDatatype == "MPI_UINT16_T"))
                              .Case("uint32_t", (mpiDatatype == "MPI_UINT32_T"))
                              .Case("uint64_t", (mpiDatatype == "MPI_UINT64_T"))
                              // unknown typedefs are rated as correct
                              .Default(true);

    if (!isTypeMatching) bugReporter_.reportTypeMismatch(callExpr);
}

/**
 * Check if float arguments are used for mpi call
 * where only integer values make sense. (count, rank, tag)
 *
 * @param mpiCall to check the arguments for
 */
void MPI_ASTVisitor::checkForInvalidArgs(const MPICall &mpiCall) const {
    if (funcClassifier_.isPointToPointType(mpiCall.identInfo_)) {
        const auto indicesToCheck = {MPIPointToPoint::kCount,
                                     MPIPointToPoint::kRank,
                                     MPIPointToPoint::kTag};

        // iterate indices which should not have float arguments
        for (const size_t idx : indicesToCheck) {
            // check for float variables
            const auto &arg = mpiCall.arguments_[idx];
            const auto &vars = arg.vars_;
            for (const auto &var : vars) {
                vis::TypeVisitor typeVisitor{var->getType()};
                if (!typeVisitor.builtinType_ ||
                    !typeVisitor.builtinType_->isIntegerType()) {
                    bugReporter_.reportInvalidArgumentType(
                        mpiCall.callExpr_, idx, InvalidArgType::kVariable);
                }
            }

            // check for float literals
            if (arg.floatingLiterals_.size()) {
                bugReporter_.reportInvalidArgumentType(
                    mpiCall.callExpr_, idx, InvalidArgType::kLiteral);
            }

            // check for float return values from functions
            const auto &functions = arg.functions_;
            for (const auto &function : functions) {
                vis::TypeVisitor typeVisitor{function->getReturnType()};
                if (!typeVisitor.builtinType_ ||
                    !typeVisitor.builtinType_->isIntegerType()) {
                    bugReporter_.reportInvalidArgumentType(
                        mpiCall.callExpr_, idx, InvalidArgType::kReturnType);
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

void MPI_ASTVisitor::checkForDuplicatePointToPoint(
    const MPICall &callToCheck) const {
    for (const MPICall &comparedCall : MPICall::visitedCalls) {
        // to omit double matching
        if (comparedCall.isMarked_) continue;
        // to ensure mpi point to point call is matched against
        if (!funcClassifier_.isPointToPointType(comparedCall.identInfo_))
            continue;
        // do not compare with the call itself
        if (callToCheck.id_ == comparedCall.id_) continue;
        // both must be of send or receive type
        if (funcClassifier_.isSendType(callToCheck.identInfo_) !=
            funcClassifier_.isSendType(comparedCall.identInfo_))
            continue;

        // argument types which are compared by all 'components' –––––––
        bool identical = true;
        const auto indicesToCheck = {MPIPointToPoint::kCount,
                                     MPIPointToPoint::kRank,
                                     MPIPointToPoint::kTag};
        for (const size_t idx : indicesToCheck) {
            if (!fullArgumentComparison(callToCheck, comparedCall, idx)) {
                identical = false;
                break;  // end inner loop
            }
        }
        if (!identical) continue;

        // compare specified mpi datatypes –––––––––––––––––––––––––––––
        const VarDecl *mpiTypeNew =
            callToCheck.arguments_[MPIPointToPoint::kDatatype].vars_.front();
        const VarDecl *mpiTypePrev =
            comparedCall.arguments_[MPIPointToPoint::kDatatype].vars_.front();

        // VarDecl->getName() returns implementation defined type name:
        // ompi_mpi_xy
        if (mpiTypeNew->getName() != mpiTypePrev->getName()) continue;

        // mark call to omit symmetric duplicate report
        callToCheck.isMarked_ = true;

        // if function reaches this point all arguments have been equal
        bugReporter_.reportDuplicate(callToCheck.callExpr_,
                                     comparedCall.callExpr_);

        // do not match against other calls
        // nevertheless all duplicate calls will appear in the diagnostics
        // due to transitivity of duplicates
        return;
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

    // unmark calls
    for (const MPICall &mpiCall : MPICall::visitedCalls) {
        if (funcClassifier_.isPointToPointType(mpiCall.identInfo_)) {
            mpiCall.isMarked_ = false;
        }
    }
}

// host class ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
/**
 * Main checker host class. Registers checker functionality.
 * Class name determines checker name to specify when the command line
 * is invoked for static analysis.
 * Receives callback for every translation unit about to visit.
 */
class MPISchemaChecker : public Checker<check::ASTDecl<TranslationUnitDecl>> {
public:
    void checkASTDecl(const TranslationUnitDecl *tuDecl,
                      AnalysisManager &analysisManager,
                      BugReporter &bugReporter) const {
        MPI_ASTVisitor visitor{bugReporter, *this, analysisManager};
        visitor.TraverseTranslationUnitDecl(
            const_cast<TranslationUnitDecl *>(tuDecl));

        // invoked after travering a translation unit
        visitor.checkForDuplicates();
    }
};

}  // end of namespace: mpi

void ento::registerMPISchemaChecker(CheckerManager &mgr) {
    mgr.registerChecker<mpi::MPISchemaChecker>();
}
