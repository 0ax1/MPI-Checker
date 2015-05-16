#include "MPICheckerAST.hpp"

using namespace clang;
using namespace ento;

namespace mpi {

void MPICheckerAST::checkForCollectiveCall(const MPICall &mpiCall) const {
    if (funcClassifier_.isCollectiveType(mpiCall)) {
        bugReporter_.reportCollCallInBranch(mpiCall.callExpr_);
    }
}

/**
 * Iterates rank cases looking for point to point send or receive
 * functions. If found report them as unmatched.
 *
 * @param rankCases
 */
void MPICheckerAST::checkUnmatchedCalls(
    const llvm::SmallVectorImpl<MPIRankCase> &rankCases) const {
    for (const MPIRankCase &rankCase : rankCases) {
        for (const MPICall &call : rankCase.mpiCalls_) {
            if (funcClassifier_.isSendType(call)) {
                bugReporter_.reportUnmatchedCall(call.callExpr_, "receive");
            } else if (funcClassifier_.isRecvType(call)) {
                bugReporter_.reportUnmatchedCall(call.callExpr_, "send");
            }
        }
    }
}

/**
 * Strips calls from two different rank cases where the mpi send and receive
 * functions match. This respects the order of functions inside the cases. While
 * nonblocking calls are just skipped in case of a mismatch, blocking calls on
 * the recv side that do not match quit the loop.
 *
 * @param rankCase1
 * @param rankCase2
 */
void MPICheckerAST::matchRankCasePair(MPIRankCase &rankCase1,
                                      MPIRankCase &rankCase2) {
    for (size_t i = 0; i < rankCase2.size() && rankCase1.size(); ++i) {
        // skip non point to point
        while (
            !funcClassifier_.isPointToPointType(rankCase1.mpiCalls_[0].get())) {
            cont::eraseIndex(rankCase1.mpiCalls_, 0);
            if (!rankCase1.size()) return;
        }
        if (isSendRecvPair(rankCase1.mpiCalls_[0], rankCase2.mpiCalls_[i])) {
            cont::eraseIndex(rankCase1.mpiCalls_, 0);
            cont::eraseIndex(rankCase2.mpiCalls_, i--);
        }
        // if non-matching, blocking function is hit, break
        else if (funcClassifier_.isBlockingType(rankCase2.mpiCalls_[i].get())) {
            break;
        }
    }
}

void MPICheckerAST::checkPointToPointSchema() {
    auto &rankCases = MPIRankCase::visitedRankCases;

    for (size_t i = 0; i < 4; ++i) {
        for (MPIRankCase &rankCase1 : rankCases) {
            for (MPIRankCase &rankCase2 : rankCases) {
                // rank cases must be distinct
                if (!rankCase1.isRankConditionEqual(rankCase2) &&
                    &rankCase1 != &rankCase2) {
                    // rank cases are potential partner
                    matchRankCasePair(rankCase1, rankCase2);
                }
            }
        }
    }
    checkUnmatchedCalls(rankCases);
}

// vergleichen von argumenten
// falls alles plus erlaube beliebige permutation
// falls minus enthalten muss argument identisch sein
// muss identisch sein weil man nicht weiß, welche neg/pos sind

/**
 * Check if two calls are a send/recv pair.
 *
 * @param sendCall
 * @param recvCall
 *
 * @return if they are send/recv pair
 */
bool MPICheckerAST::isSendRecvPair(const MPICall &sendCall,
                                   const MPICall &recvCall) const {
    if (!funcClassifier_.isSendType(sendCall)) return false;
    if (!funcClassifier_.isRecvType(recvCall)) return false;

    // compare mpi datatype
    llvm::StringRef sendDataType = util::sourceRangeAsStringRef(
        sendCall.arguments_[MPIPointToPoint::kDatatype].stmt_->getSourceRange(),
        analysisManager_);

    llvm::StringRef recvDataType = util::sourceRangeAsStringRef(
        recvCall.arguments_[MPIPointToPoint::kDatatype].stmt_->getSourceRange(),
        analysisManager_);

    if (sendDataType != recvDataType) return false;

    // compare count, tag
    for (size_t idx : {MPIPointToPoint::kCount, MPIPointToPoint::kTag}) {
        if (sendCall.arguments_[idx].containsNonCommutativeOps()) {
            if (!sendCall.arguments_[idx].isEqualOrdered(
                    sendCall.arguments_[idx], true)) {
                return false;
            }
        } else {
            if (!sendCall.arguments_[idx].isEqualPermutative(
                    sendCall.arguments_[idx], true)) {
                return false;
            }
        }
    }

    // compare ranks
    // exclude operator from this comparison
    if (!sendCall.arguments_[MPIPointToPoint::kRank].isEqualOrdered(
            sendCall.arguments_[MPIPointToPoint::kRank], false)) {
        return false;
    }

    // compare rank operators
    auto &rankArgSend = sendCall.arguments_[MPIPointToPoint::kRank];
    auto &rankArgRecv = recvCall.arguments_[MPIPointToPoint::kRank];
    auto &operatorsSend = rankArgSend.binaryOperators_;
    auto &operatorsRecv = rankArgRecv.binaryOperators_;

    if (operatorsSend.size() != operatorsRecv.size()) {
        return false;
    }

    // operators except last one must be equal
    for (size_t i = 0; i < operatorsSend.size()-1; ++i) {
        if (operatorsSend[i] != operatorsRecv[i]) {
            return false;
        }
    }

    // last operator must be inverse
    if (!((BinaryOperatorKind::BO_Add == operatorsSend.back() &&
           BinaryOperatorKind::BO_Sub == operatorsRecv.back()) ||

          (BinaryOperatorKind::BO_Sub == operatorsSend.back() &&
           BinaryOperatorKind::BO_Add == operatorsRecv.back())))
        return false;


    return true;
}

/**
 * Checks if buffer type and specified mpi datatype matches.
 *
 * @param mpiCall call to check type correspondence for
 */
void MPICheckerAST::checkBufferTypeMatch(const MPICall &mpiCall) const {
    // one pair consists of {bufferIdx, mpiDatatypeIdx}
    llvm::SmallVector<std::pair<size_t, size_t>, 2> indexPairs;

    if (funcClassifier_.isPointToPointType(mpiCall)) {
        indexPairs.push_back(
            {MPIPointToPoint::kBuf, MPIPointToPoint::kDatatype});
    } else if (funcClassifier_.isCollectiveType(mpiCall)) {
        if (funcClassifier_.isReduceType(mpiCall)) {
            // only check buffer type if not inplace
            if (util::sourceRangeAsStringRef(
                    mpiCall.callExpr_->getArg(0)->getSourceRange(),
                    analysisManager_) != "MPI_IN_PLACE") {
                indexPairs.push_back({0, 3});
            }
            indexPairs.push_back({1, 3});
        } else if (funcClassifier_.isScatterType(mpiCall) ||
                   funcClassifier_.isGatherType(mpiCall) ||
                   funcClassifier_.isAlltoallType(mpiCall)) {
            indexPairs.push_back({0, 2});
            indexPairs.push_back({3, 5});
        } else if (funcClassifier_.isBcastType(mpiCall)) {
            indexPairs.push_back({0, 2});
        }
    }

    // for every buffer mpi-data pair in function
    // check if their types match
    for (const auto &idxPair : indexPairs) {
        const VarDecl *bufferArg =
            mpiCall.arguments_[idxPair.first].vars_.front();

        // collect buffer type information
        const mpi::TypeVisitor typeVisitor{bufferArg->getType()};

        // get mpi datatype as string
        auto mpiDatatype = mpiCall.arguments_[idxPair.second].stmt_;
        StringRef mpiDatatypeString{util::sourceRangeAsStringRef(
            mpiDatatype->getSourceRange(), analysisManager_)};

        selectTypeMatcher(typeVisitor, mpiCall, mpiDatatypeString, idxPair);
    }
}

/**
 * Select apprioriate function to match the buffer type against
 * the specified mpi datatype.
 *
 * @param typeVisitor contains information about the buffer
 * @param mpiCall call whose arguments are observed
 * @param mpiDatatypeString
 * @param idxPair bufferIdx, mpiDatatypeIdx
 */
void MPICheckerAST::selectTypeMatcher(
    const mpi::TypeVisitor &typeVisitor, const MPICall &mpiCall,
    const StringRef mpiDatatypeString,
    const std::pair<size_t, size_t> &idxPair) const {
    clang::BuiltinType *builtinTypeBuffer = typeVisitor.builtinType_;
    bool isTypeMatching{true};

    // check for exact width types (e.g. int16_t, uint32_t)
    if (typeVisitor.isTypedefType_) {
        isTypeMatching = matchExactWidthType(typeVisitor, mpiDatatypeString);
    }
    // check for complex-floating types (e.g. float _Complex)
    else if (typeVisitor.complexType_) {
        isTypeMatching = matchComplexType(typeVisitor, mpiDatatypeString);
    }
    // check for basic builtin types (e.g. int, char)
    else if (!builtinTypeBuffer)
        return;  // if no builtin type cancel checking
    else if (builtinTypeBuffer->isBooleanType()) {
        isTypeMatching = matchBoolType(typeVisitor, mpiDatatypeString);
    } else if (builtinTypeBuffer->isAnyCharacterType()) {
        isTypeMatching = matchCharType(typeVisitor, mpiDatatypeString);
    } else if (builtinTypeBuffer->isSignedInteger()) {
        isTypeMatching = matchSignedType(typeVisitor, mpiDatatypeString);
    } else if (builtinTypeBuffer->isUnsignedIntegerType()) {
        isTypeMatching = matchUnsignedType(typeVisitor, mpiDatatypeString);
    } else if (builtinTypeBuffer->isFloatingType()) {
        isTypeMatching = matchFloatType(typeVisitor, mpiDatatypeString);
    }

    if (!isTypeMatching)
        bugReporter_.reportTypeMismatch(mpiCall.callExpr_, idxPair);
}

bool MPICheckerAST::matchBoolType(const mpi::TypeVisitor &visitor,
                                  const llvm::StringRef mpiDatatype) const {
    return (mpiDatatype == "MPI_C_BOOL");
}

bool MPICheckerAST::matchCharType(const mpi::TypeVisitor &visitor,
                                  const llvm::StringRef mpiDatatype) const {
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

    return isTypeMatching;
}

bool MPICheckerAST::matchSignedType(const mpi::TypeVisitor &visitor,
                                    const llvm::StringRef mpiDatatype) const {
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

    return isTypeMatching;
}

bool MPICheckerAST::matchUnsignedType(const mpi::TypeVisitor &visitor,
                                      const llvm::StringRef mpiDatatype) const {
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
    return isTypeMatching;
}

bool MPICheckerAST::matchFloatType(const mpi::TypeVisitor &visitor,
                                   const llvm::StringRef mpiDatatype) const {
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
    return isTypeMatching;
}

bool MPICheckerAST::matchComplexType(const mpi::TypeVisitor &visitor,
                                     const llvm::StringRef mpiDatatype) const {
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

    return isTypeMatching;
}

bool MPICheckerAST::matchExactWidthType(
    const mpi::TypeVisitor &visitor, const llvm::StringRef mpiDatatype) const {
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

    return isTypeMatching;
}

/**
 * Check if invalid argument types are used in a mpi call.
 * This check looks at indices where only integer values are valid.
 * (count, rank, tag) Any non integer type usage is reported.
 *
 * @param mpiCall to check the arguments for
 */
void MPICheckerAST::checkForInvalidArgs(const MPICall &mpiCall) const {
    if (funcClassifier_.isPointToPointType(mpiCall)) {
        const auto indicesToCheck = {MPIPointToPoint::kCount,
                                     MPIPointToPoint::kRank,
                                     MPIPointToPoint::kTag};

        // iterate indices which should not have float arguments
        for (const size_t idx : indicesToCheck) {
            // check for invalid variable types
            const auto &arg = mpiCall.arguments_[idx];
            const auto &vars = arg.vars_;
            for (const auto &var : vars) {
                const mpi::TypeVisitor typeVisitor{var->getType()};
                if (!typeVisitor.builtinType_ ||
                    !typeVisitor.builtinType_->isIntegerType()) {
                    bugReporter_.reportInvalidArgumentType(
                        mpiCall.callExpr_, idx, var->getSourceRange(),
                        "Variable");
                }
            }

            // check for float literals
            if (arg.floatingLiterals_.size()) {
                bugReporter_.reportInvalidArgumentType(
                    mpiCall.callExpr_, idx,
                    arg.floatingLiterals_.front()->getSourceRange(), "Literal");
            }

            // check for invalid return types from functions
            const auto &functions = arg.functions_;
            for (const auto &function : functions) {
                const mpi::TypeVisitor typeVisitor{function->getReturnType()};
                if (!typeVisitor.builtinType_ ||
                    !typeVisitor.builtinType_->isIntegerType()) {
                    bugReporter_.reportInvalidArgumentType(
                        mpiCall.callExpr_, idx, function->getSourceRange(),
                        "Return value");
                }
            }
        }
    }
}

bool MPICheckerAST::areDatatypesEqual(const MPICall &callOne,
                                      const MPICall &callTwo,
                                      const size_t idx) const {
    const VarDecl *mpiTypeNew = callOne.arguments_[idx].vars_.front();
    const VarDecl *mpiTypePrev = callTwo.arguments_[idx].vars_.front();

    return mpiTypeNew->getName() == mpiTypePrev->getName();
}

/**
 * Check if two calls qualify for a redundancy check.
 *
 * @param callToCheck
 * @param comparedCall
 *
 * @return
 */
bool MPICheckerAST::qualifyRedundancyCheck(const MPICall &callToCheck,
                                           const MPICall &comparedCall) const {
    if (comparedCall.isMarked_) return false;  // to omit double matching
    // do not compare with the call itself
    if (callToCheck.id_ == comparedCall.id_) return false;
    if (!((funcClassifier_.isPointToPointType(callToCheck) &&
           funcClassifier_.isPointToPointType(comparedCall)) ||
          (funcClassifier_.isCollectiveType(callToCheck) &&
           funcClassifier_.isCollectiveType(comparedCall))))
        return false;

    if (funcClassifier_.isPointToPointType(callToCheck)) {
        // both must be send or recv types
        return (funcClassifier_.isSendType(callToCheck) &&
                funcClassifier_.isSendType(comparedCall)) ||
               (funcClassifier_.isRecvType(callToCheck) &&
                funcClassifier_.isRecvType(comparedCall));

    } else if (funcClassifier_.isCollectiveType(callToCheck)) {
        // calls must be of the same type
        return (funcClassifier_.isScatterType(callToCheck) &&
                funcClassifier_.isScatterType(comparedCall)) ||

               (funcClassifier_.isGatherType(callToCheck) &&
                funcClassifier_.isGatherType(comparedCall)) ||

               (funcClassifier_.isAlltoallType(callToCheck) &&
                funcClassifier_.isAlltoallType(comparedCall)) ||

               (funcClassifier_.isBcastType(callToCheck) &&
                funcClassifier_.isBcastType(comparedCall)) ||

               (funcClassifier_.isReduceType(callToCheck) &&
                funcClassifier_.isReduceType(comparedCall));
    }
    return false;
}

/**
 * Check if there is a redundant call to the call passed.
 *
 * @param callToCheck
 */
void MPICheckerAST::checkForRedundantCall(const MPICall &callToCheck) const {
    // SmallVector<size_t, 3> indicesToCheckComponents;
    // SmallVector<size_t, 2> indicesToCheckAsString;

    // if (funcClassifier_.isPointToPointType(callToCheck)) {
    // indicesToCheckComponents = {MPIPointToPoint::kCount,
    // MPIPointToPoint::kRank,
    // MPIPointToPoint::kTag};
    // indicesToCheckAsString = {MPIPointToPoint::kDatatype};
    // } else if (funcClassifier_.isReduceType(callToCheck)) {
    // indicesToCheckComponents = {2};
    // indicesToCheckAsString = {3, 4};
    // } else if (funcClassifier_.isScatterType(callToCheck) ||
    // funcClassifier_.isGatherType(callToCheck) ||
    // funcClassifier_.isAlltoallType(callToCheck)) {
    // indicesToCheckComponents = {1, 4, 6};
    // indicesToCheckAsString = {2, 5};
    // } else if (funcClassifier_.isBcastType(callToCheck)) {
    // indicesToCheckComponents = {1, 3};
    // indicesToCheckAsString = {2};
    // }

    // for (const MPICall &comparedCall : MPICall::visitedCalls) {
    // if (!qualifyRedundancyCheck(callToCheck, comparedCall)) continue;

    // // argument types which are compared by all 'components' –––––––
    // bool identical = true;
    // for (const size_t idx : indicesToCheckComponents) {
    // if (!areComponentsOfArgEqual(callToCheck, comparedCall, idx)) {
    // identical = false;
    // break;  // end inner loop
    // }
    // }
    // // compare specified mpi datatypes –––––––––––––––––––––––––––––
    // for (const size_t idx : indicesToCheckAsString) {
    // if (!areDatatypesEqual(callToCheck, comparedCall, idx)) {
    // identical = false;
    // break;  // end inner loop
    // }
    // }
    // if (!identical) continue;

    // // if function reaches this point all arguments have been equal
    // // mark call to omit symmetric duplicate report
    // callToCheck.isMarked_ = true;

    // SmallVector<size_t, 5> checkedIndices;
    // cont::copy(indicesToCheckComponents, checkedIndices);
    // cont::copy(indicesToCheckAsString, checkedIndices);

    // bugReporter_.reportRedundantCall(
    // callToCheck.callExpr_, comparedCall.callExpr_, checkedIndices);

    // // do not match against further calls
    // // still all duplicate calls will appear in the diagnostics
    // // due to transitivity of duplicates
    // return;
    // }
}

/**
 * Check if there are redundant mpi calls.
 *
 * @param callEvent
 * @param mpiFnCallSet set searched for identical calls
 *
 * @return is equal call in list
 */
void MPICheckerAST::checkForRedundantCalls() const {
    for (const MPICall &mpiCall : MPICall::visitedCalls) {
        checkForRedundantCall(mpiCall);
    }

    // unmark calls
    for (const MPICall &mpiCall : MPICall::visitedCalls) {
        mpiCall.isMarked_ = false;
    }
}

}  // end of namespace: mpi
