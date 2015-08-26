/*
 The MIT License (MIT)

 Copyright (c) 2015 Alexander Droste

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
*/

#include "MPICheckerAST.hpp"

using namespace clang;
using namespace ento;

namespace mpi {

/**
 * Checks if point to point functions resolve to a valid schema.
 */
void MPICheckerAST::checkPointToPointSchema() const {
    MPIRankCase::unmarkCalls();

    // search send/recv pairs for interacting cases
    for (MPIRankCase &rankCase1 : MPIRankCase::cases) {
        for (MPIRankCase &rankCase2 : MPIRankCase::cases) {
            // rank conditions must be distinct or ambiguous
            if (!rankCase1.isRankUnambiguouslyEqual(rankCase2)) {
                // rank cases are potential partner
                checkSendRecvMatches(rankCase1, rankCase2);
            }
        }
    }

    // trigger report for unmarked
    for (const MPIRankCase &rankCase : MPIRankCase::cases) {
        for (const MPICall &call : rankCase.mpiCalls()) {
            if (funcClassifier_.isSendType(call) && !call.isMarked_) {
                bugReporter_.reportUnmatchedCall(call.callExpr(), "receive");
            } else if (funcClassifier_.isRecvType(call) && !call.isMarked_) {
                bugReporter_.reportUnmatchedCall(call.callExpr(), "send");
            }
        }
    }
}

/**
 * Matches send with recv operations between two rank cases.
 * For the first case send operations are tried to be matched
 * with recv operations from the second case. In case of a match
 * calls are marked.
 *
 * @param rankCase1
 * @param rankCase2
 */
void MPICheckerAST::checkSendRecvMatches(const MPIRankCase &sendCase,
                                         const MPIRankCase &recvCase) const {
    // find send/recv pairs
    for (const MPICall &send : sendCase.mpiCalls()) {
        // skip non sends for case 1
        if (!funcClassifier_.isSendType(send) || send.isMarked_) continue;

        // skip non recvs for case 2
        for (const MPICall &recv : recvCase.mpiCalls()) {
            if (!funcClassifier_.isRecvType(recv) || recv.isMarked_) continue;

            // check if pair matches
            if (isSendRecvPair(send, recv, sendCase, recvCase)) {
                send.isMarked_ = true;
                recv.isMarked_ = true;
                break;
            }
        }
    }
}

/**
 * Check if mpi functions can be reached.
 */
void MPICheckerAST::checkReachbility() const {
    MPIRankCase::unmarkCalls();

    // multiple send/recv phases per rank case are allowed
    for (int i = 0; i < 4; ++i) {
        // search send/recv pairs for interacting cases
        for (MPIRankCase &rankCase1 : MPIRankCase::cases) {
            for (MPIRankCase &rankCase2 : MPIRankCase::cases) {
                // rank conditions must be distinct or ambiguous
                if (!rankCase1.isRankUnambiguouslyEqual(rankCase2)) {
                    // rank cases are potential partner
                    checkReachbilityPair(rankCase1, rankCase2);
                }
            }
        }
    }

    // trigger report for unreached
    for (const MPIRankCase &rankCase : MPIRankCase::cases) {
        for (const MPICall &call : rankCase.mpiCalls()) {
            if (funcClassifier_.isMPIType(call) && !call.isReachable_) {
                bugReporter_.reportNotReachableCall(call.callExpr());
            }
        }
    }
}

/**
 * Check for reachability of calls between two cases.
 *
 * @param firstCase
 * @param secondCase
 */
void MPICheckerAST::checkReachbilityPair(const MPIRankCase &sendCase,
                                         const MPIRankCase &recvCase) const {
    // find send/recv pairs
    for (const MPICall &send : sendCase.mpiCalls()) {
        send.isReachable_ = true;
        if (send.isMarked_) continue;

        for (const MPICall &recv : recvCase.mpiCalls()) {
            recv.isReachable_ = true;
            if (recv.isMarked_) continue;

            // check if pair matches
            if (isSendRecvPair(send, recv, sendCase, recvCase)) {
                send.isMarked_ = true;
                recv.isMarked_ = true;
                break;
            }
            // no match and call was blocking
            else if (funcClassifier_.isBlockingType(recv)) {
                break;
            }
        }

        // no matching recv found in second case
        if (funcClassifier_.isBlockingType(send) && !send.isMarked_) {
            return;
        }
    }
}

/**
 * Checks for collective calls in rank case. Triggers bug reporter.
 *
 * @param mpiCall
 */
void MPICheckerAST::checkForCollectiveCalls(const MPIRankCase &rankCase) const {
    for (const MPICall &call : rankCase.mpiCalls()) {
        if (funcClassifier_.isCollectiveType(call)) {
            bugReporter_.reportCollCallInBranch(call.callExpr());
        }
    }
}

/**
 * Checks if mpi-datatypes for 2 different point to point calls are equal.
 *
 * @param sendCall
 * @param recvCall
 *
 * @return equality
 */
bool MPICheckerAST::areDatatypesEqual(const MPICall &sendCall,
                                      const MPICall &recvCall) const {
    // compare mpi datatype
    llvm::StringRef sendDataType = util::sourceRangeAsStringRef(
        sendCall.arguments()[MPIPointToPoint::kDatatype]
            .stmt()
            ->getSourceRange(),
        analysisManager_);

    llvm::StringRef recvDataType = util::sourceRangeAsStringRef(
        recvCall.arguments()[MPIPointToPoint::kDatatype]
            .stmt()
            ->getSourceRange(),
        analysisManager_);

    return sendDataType == recvDataType;
}

/**
 * Checks if two (point to point) calls are a send/recv pair.
 *
 * @param sendCall
 * @param recvCall
 *
 * @return if they are send/recv pair
 */
bool MPICheckerAST::isSendRecvPair(const MPICall &sendCall,
                                   const MPICall &recvCall,
                                   const MPIRankCase &sendCase,
                                   const MPIRankCase &recvCase) const {
    if (!funcClassifier_.isSendType(sendCall)) return false;
    if (!funcClassifier_.isRecvType(recvCall)) return false;
    if (!areDatatypesEqual(sendCall, recvCall)) return false;

    // compare count, tag
    for (const size_t idx : {MPIPointToPoint::kCount, MPIPointToPoint::kTag}) {
        if (sendCall.arguments()[idx] != recvCall.arguments()[idx]) {
            return false;
        }
    }

    // compare ranks
    const auto &rankArgSend = sendCall.arguments()[MPIPointToPoint::kRank];
    const auto &rankArgRecv = recvCall.arguments()[MPIPointToPoint::kRank];

    // match special cases---------------------
    llvm::SmallVector<std::string, 1> firstRank{"0"};
    llvm::SmallVector<std::string, 3> lastRank{"-", MPIProcessCount::encoding,
                                               "1"};

    // first to last match
    if (sendCase.isFirstRank() && rankArgSend.valueSequence() == lastRank &&
        recvCase.isLastRank() && rankArgRecv.valueSequence() == firstRank) {
        return true;
    }
    // last to first match
    else if (sendCase.isLastRank() &&
             rankArgSend.valueSequence() == firstRank &&
             recvCase.isFirstRank() &&
             rankArgRecv.valueSequence() == lastRank) {
        return true;
    }
    //------------------------------------------
    if (rankArgSend.typeSequence().size() !=
            rankArgRecv.typeSequence().size() ||
        rankArgSend.valueSequence().size() !=
            rankArgRecv.valueSequence().size())
        return false;

    // build sequences without last operator(skip first element)
    const llvm::SmallVector<ArgumentVisitor::ComponentType, 4> sendTypeSeq(
        rankArgSend.typeSequence().begin() + 1,
        rankArgSend.typeSequence().end()),
        recvTypeSeq{rankArgRecv.typeSequence().begin() + 1,
                    rankArgRecv.typeSequence().end()};

    const llvm::SmallVector<std::string, 4> sendValSeq{
        rankArgSend.valueSequence().begin() + 1,
        rankArgSend.valueSequence().end()},
        recvValSeq{rankArgRecv.valueSequence().begin() + 1,
                   rankArgRecv.valueSequence().end()};

    bool containsSubtraction{false};
    for (size_t i = 0; i < sendValSeq.size(); ++i) {
        if (sendValSeq[i] == "-" || recvValSeq[i] == "-") {
            containsSubtraction = true;
            break;
        }
    }

    // check ordered
    if (containsSubtraction &&
        (sendTypeSeq != recvTypeSeq || sendValSeq != recvValSeq)) {
        return false;
    }
    // check permutation
    if (!containsSubtraction &&
        ((!cont::isPermutation(sendTypeSeq, recvTypeSeq)) ||
         (!cont::isPermutation(sendValSeq, recvValSeq)))) {
        return false;
    }
    // last (value|var|function) must be identical
    if (sendValSeq.back() != recvValSeq.back()) return false;

    // last operator must be inverse
    if (!rankArgSend.isLastOperatorInverse(rankArgRecv)) return false;

    return true;
}

/**
 * Checks if buffer type and specified mpi datatype matches.
 *
 * @param mpiCall call to check type correspondence for
 */
void MPICheckerAST::checkBufferTypeMatch(const MPICall &mpiCall) const {
    // one pair consists of {bufferIdx, mpiDatatypeIdx}
    IndexPairs indexPairs = bufferDataTypeIndices(mpiCall);

    // for every buffer mpi-data pair in function
    // check if their types match
    for (const auto &idxPair : indexPairs) {
        // wave through uncaptured data
        if (!mpiCall.arguments()[idxPair.first].combinedVars().size()) continue;

        const ValueDecl *bufferArg =
            mpiCall.arguments()[idxPair.first].combinedVars().front();

        // collect buffer type information
        const mpi::TypeVisitor typeVisitor{bufferArg->getType()};

        // get mpi datatype as string
        auto mpiDatatype = mpiCall.arguments()[idxPair.second].stmt();
        StringRef mpiDatatypeString{util::sourceRangeAsStringRef(
            mpiDatatype->getSourceRange(), analysisManager_)};

        // MPI_BYTE needs no matching
        if (mpiDatatypeString == "MPI_BYTE") return;

        selectTypeMatcher(typeVisitor, mpiCall, mpiDatatypeString, idxPair);
    }
}

/**
 * Returns index pairs for each buffer, datatype pair.
 *
 * @param mpiCall
 *
 * @return index pairs
 */
MPICheckerAST::IndexPairs MPICheckerAST::bufferDataTypeIndices(
    const MPICall &mpiCall) const {
    IndexPairs indexPairs;

    if (funcClassifier_.isPointToPointType(mpiCall)) {
        indexPairs.push_back(
            {MPIPointToPoint::kBuf, MPIPointToPoint::kDatatype});
    } else if (funcClassifier_.isCollectiveType(mpiCall)) {
        if (funcClassifier_.isReduceType(mpiCall)) {
            // only check buffer type if not inplace
            if (util::sourceRangeAsStringRef(
                    mpiCall.callExpr()->getArg(0)->getSourceRange(),
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
    return indexPairs;
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
    const clang::BuiltinType *builtinType = typeVisitor.builtinType();
    bool isTypeMatching{true};

    // check for exact width types (e.g. int16_t, uint32_t)
    if (typeVisitor.isTypedefType()) {
        isTypeMatching = matchExactWidthType(typeVisitor, mpiDatatypeString);
    }
    // check for complex-floating types (e.g. float _Complex)
    else if (typeVisitor.isComplexType()) {
        isTypeMatching = matchComplexType(typeVisitor, mpiDatatypeString);
    }
    // check for basic builtin types (e.g. int, char)
    else if (!builtinType) {
        return;  // if no builtin type cancel checking
    } else if (builtinType->isBooleanType()) {
        isTypeMatching = matchBoolType(typeVisitor, mpiDatatypeString);
    } else if (builtinType->isAnyCharacterType()) {
        isTypeMatching = matchCharType(typeVisitor, mpiDatatypeString);
    } else if (builtinType->isSignedInteger()) {
        isTypeMatching = matchSignedType(typeVisitor, mpiDatatypeString);
    } else if (builtinType->isUnsignedIntegerType()) {
        isTypeMatching = matchUnsignedType(typeVisitor, mpiDatatypeString);
    } else if (builtinType->isFloatingType()) {
        isTypeMatching = matchFloatType(typeVisitor, mpiDatatypeString);
    }

    if (!isTypeMatching)
        bugReporter_.reportTypeMismatch(mpiCall.callExpr(), idxPair);
}

bool MPICheckerAST::matchBoolType(const mpi::TypeVisitor &visitor,
                                  const llvm::StringRef mpiDatatype) const {
    return (mpiDatatype == "MPI_C_BOOL");
}

bool MPICheckerAST::matchCharType(const mpi::TypeVisitor &visitor,
                                  const llvm::StringRef mpiDatatype) const {
    bool isTypeMatching;
    switch (visitor.builtinType()->getKind()) {
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

    switch (visitor.builtinType()->getKind()) {
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

    switch (visitor.builtinType()->getKind()) {
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

    switch (visitor.builtinType()->getKind()) {
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

    switch (visitor.builtinType()->getKind()) {
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
    bool isTypeMatching = llvm::StringSwitch<bool>(visitor.typedefTypeName())
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
    llvm::SmallVector<size_t, 1> indicesToCheck{integerIndices(mpiCall)};
    if (!indicesToCheck.size()) return;

    // iterate indices which should not have integer arguments
    const CallExpr *const callExpr = mpiCall.callExpr();
    for (const size_t idx : indicesToCheck) {
        if (!callExpr->getArg(idx)->IgnoreCasts()->getType()->isIntegerType()) {
            bugReporter_.reportInvalidArgumentType(mpiCall.callExpr(), idx);
        }
    }
}

/**
 * Return an array of indices that must be integer values for a given call.
 *
 * @param mpiCall
 *
 * @return int indices
 */
llvm::SmallVector<size_t, 1> MPICheckerAST::integerIndices(
    const MPICall &mpiCall) const {
    llvm::SmallVector<size_t, 1> intIndices;

    if (funcClassifier_.isPointToPointType(mpiCall)) {
        intIndices = {MPIPointToPoint::kCount, MPIPointToPoint::kRank,
                      MPIPointToPoint::kTag};
    } else if (funcClassifier_.isScatterType(mpiCall) ||
               funcClassifier_.isGatherType(mpiCall)) {
        if (funcClassifier_.isAllgatherType(mpiCall)) {
            intIndices = {1, 4};
        } else {
            intIndices = {1, 4, 6};
        }
    } else if (funcClassifier_.isAlltoallType(mpiCall)) {
        intIndices = {1, 4};
    } else if (funcClassifier_.isReduceType(mpiCall)) {
        if (funcClassifier_.isCollToColl(mpiCall)) {
            intIndices = {2};
        } else {
            intIndices = {2, 5};
        }
    } else if (funcClassifier_.isBcastType(mpiCall)) {
        intIndices = {1, 3};
    }

    return intIndices;
}

/**
 * Set function currently visited to pass to bug reporter
 * in case of invariant violation.
 *
 * @param functionDecl current function
 */
void MPICheckerAST::setCurrentlyVisitedFunction(
    const clang::FunctionDecl *const functionDecl) {
    bugReporter_.currentFunctionDecl_ = functionDecl;
}

// TODO check if there are any rank case partners at all

}  // end of namespace: mpi
