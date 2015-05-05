#include <utility>
#include "llvm/ADT/SmallVector.h"
#include "clang/Lex/Lexer.h"

#include "MPIChecker.hpp"
#include "Container.hpp"
#include "Utility.hpp"

using namespace clang;
using namespace ento;

namespace mpi {

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

    // captures all visited calls traversing the ast
    static llvm::SmallVector<MPICall, 16> visitedCalls;

private:
    static unsigned long id;
};
llvm::SmallVector<MPICall, 16> MPICall::visitedCalls;
unsigned long MPICall::id{0};


// visitor –––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
bool MPIVisitor::VisitDecl(Decl *declaration) {
    // std::cout << declaration->getDeclKindName() << std::endl;
    return true;
}

bool MPIVisitor::VisitFunctionDecl(FunctionDecl *functionDecl) {
    // to keep track which function implementation is currently analysed
    if (functionDecl->clang::Decl::hasBody() && !functionDecl->isInlined()) {
        // to make display of function in diagnostics available
        bugReporter_.currentFunctionDecl_ = functionDecl;
    }
    return true;
}

bool MPIVisitor::VisitDeclRefExpr(DeclRefExpr *expression) { return true; }

bool MPIVisitor::VisitIfStmt(IfStmt *ifStmt) { return true; }

/**
 * Visited when function calls to execute are visited.
 *
 * @param callExpr
 *
 * @return
 */
bool MPIVisitor::VisitCallExpr(CallExpr *callExpr) {
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
        // check correctness for single calls
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
void MPIVisitor::checkBufferTypeMatch(const MPICall &mpiCall) const {
    // TODO extend to collective call
    if (funcClassifier_.isPointToPointType(mpiCall.identInfo_)) {
        const VarDecl *bufferArg =
            mpiCall.arguments_[MPIPointToPoint::kBuf].vars_.front();

        // collect type information
        vis::TypeVisitor typeVisitor{bufferArg->getType()};

        // get mpi datatype as string
        auto mpiDatatype = mpiCall.arguments_[MPIPointToPoint::kDatatype].expr_;
        StringRef mpiDatatypeString{util::sourceRangeAsStringRef(
            mpiDatatype->getSourceRange(), analysisManager_)};

        // check for exact width types (e.g. int16_t, uint32_t)
        if (typeVisitor.isTypedefType_) {
            matchExactWidthType(mpiCall.callExpr_, typeVisitor,
                                mpiDatatypeString);
            return;
        }

        // check for complex-floating types (e.g. float _Complex)
        if (typeVisitor.complexType_) {
            matchComplexType(mpiCall.callExpr_, typeVisitor, mpiDatatypeString);
            return;
        }

        // check for basic builtin types (e.g. int, char)
        clang::BuiltinType *builtinTypeBuffer = typeVisitor.builtinType_;
        if (!builtinTypeBuffer) return;  // if no builtin type cancel checking

        if (builtinTypeBuffer->isBooleanType()) {
            matchBoolType(mpiCall.callExpr_, typeVisitor, mpiDatatypeString);
        } else if (builtinTypeBuffer->isAnyCharacterType()) {
            matchCharType(mpiCall.callExpr_, typeVisitor, mpiDatatypeString);
        } else if (builtinTypeBuffer->isSignedInteger()) {
            matchSignedType(mpiCall.callExpr_, typeVisitor, mpiDatatypeString);
        } else if (builtinTypeBuffer->isUnsignedIntegerType()) {
            matchUnsignedType(mpiCall.callExpr_, typeVisitor,
                              mpiDatatypeString);
        } else if (builtinTypeBuffer->isFloatingType()) {
            matchFloatType(mpiCall.callExpr_, typeVisitor, mpiDatatypeString);
        }
    }
}
void MPIVisitor::matchBoolType(CallExpr *callExpr, vis::TypeVisitor &visitor,
                               llvm::StringRef mpiDatatype) const {
    bool isTypeMatching = (mpiDatatype == "MPI_C_BOOL");
    if (!isTypeMatching) bugReporter_.reportTypeMismatch(callExpr);
}

void MPIVisitor::matchCharType(CallExpr *callExpr, vis::TypeVisitor &visitor,
                               llvm::StringRef mpiDatatype) const {
    bool isTypeMatching;
    switch (visitor.builtinType_->getKind()) {
        case BuiltinType::SChar:
            isTypeMatching =
                (mpiDatatype == "MPI_CHAR" || mpiDatatype == "MPI_SIGNED_CHAR");
            break;
        case BuiltinType::Char_S:
            isTypeMatching =
                (mpiDatatype == "MPI_CHAR" || mpiDatatype == "MPI_SIGNED_CHAR");
            break;
        case BuiltinType::UChar:
            isTypeMatching = (mpiDatatype == "MPI_UNSIGNED_CHAR");
            break;
        case BuiltinType::Char_U:
            isTypeMatching = (mpiDatatype == "MPI_UNSIGNED_CHAR");
            break;
        case BuiltinType::WChar_S:
            isTypeMatching = (mpiDatatype == "MPI_WCHAR");
            break;
        case BuiltinType::WChar_U:
            isTypeMatching = (mpiDatatype == "MPI_WCHAR");
            break;

        default:
            isTypeMatching = true;
    }

    if (!isTypeMatching) bugReporter_.reportTypeMismatch(callExpr);
}

void MPIVisitor::matchSignedType(CallExpr *callExpr, vis::TypeVisitor &visitor,
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
        default:
            isTypeMatching = true;
    }

    if (!isTypeMatching) bugReporter_.reportTypeMismatch(callExpr);
}

void MPIVisitor::matchUnsignedType(CallExpr *callExpr,
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

void MPIVisitor::matchFloatType(CallExpr *callExpr, vis::TypeVisitor &visitor,
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

void MPIVisitor::matchComplexType(CallExpr *callExpr, vis::TypeVisitor &visitor,
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

void MPIVisitor::matchExactWidthType(CallExpr *callExpr,
                                     vis::TypeVisitor &visitor,
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
void MPIVisitor::checkForInvalidArgs(const MPICall &mpiCall) const {
    if (funcClassifier_.isPointToPointType(mpiCall.identInfo_)) {
        const auto indicesToCheck = {MPIPointToPoint::kCount,
                                     MPIPointToPoint::kRank,
                                     MPIPointToPoint::kTag};

        // iterate indices which should not have float arguments
        for (const size_t idx : indicesToCheck) {
            // check for invalid variable types
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

            // check for invalid return types from functions
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
bool MPIVisitor::fullArgumentComparison(const MPICall &callOne,
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

void MPIVisitor::checkForDuplicatePointToPoint(
    const MPICall &callToCheck) const {
    for (const MPICall &comparedCall : MPICall::visitedCalls) {
        // to omit double matching
        if (comparedCall.isMarked_) continue;
        // do not compare with the call itself
        if (callToCheck.id_ == comparedCall.id_) continue;
        // to ensure mpi point to point call is matched against
        if (!funcClassifier_.isPointToPointType(comparedCall.identInfo_))
            continue;
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
void MPIVisitor::checkForDuplicates() const {
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
class MPIChecker : public Checker<check::ASTDecl<TranslationUnitDecl>> {
public:
    void checkASTDecl(const TranslationUnitDecl *tuDecl,
                      AnalysisManager &analysisManager,
                      BugReporter &bugReporter) const {
        MPIVisitor visitor{bugReporter, *this, analysisManager};
        visitor.TraverseTranslationUnitDecl(
            const_cast<TranslationUnitDecl *>(tuDecl));

        // invoked after travering a translation unit
        visitor.checkForDuplicates();
    }
};

}  // end of namespace: mpi

void ento::registerMPIChecker(CheckerManager &mgr) {
    mgr.registerChecker<mpi::MPIChecker>();
}
