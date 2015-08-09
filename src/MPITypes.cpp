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

#include "MPITypes.hpp"
#include "Container.hpp"

using namespace clang;
using namespace ento;

namespace mpi {

unsigned long MPICall::idCounter{0};

namespace MPIRank {
llvm::SmallSet<const ValueDecl *, 4> variables;
const std::string encoding{"_rank_var_encoding_"};
}

namespace MPIProcessCount {
llvm::SmallSet<const ValueDecl *, 4> variables;
const std::string encoding{"_prc_var_encoding_"};
}

std::list<MPIRankCase> MPIRankCase::cases;

bool MPICall::operator==(const MPICall &callToCompare) const {
    if (arguments_.size() != callToCompare.arguments_.size()) return false;
    for (size_t i = 0; i < arguments_.size(); ++i) {
        if (arguments_[i] != callToCompare.arguments_[i]) {
            return false;
        }
    }
    return true;
}

bool MPICall::operator!=(const MPICall &callToCompare) const {
    return !(*this == callToCompare);
}

/**
 * Init function shared by ctors.
 * @param callExpr mpi call captured
 */
void MPICall::init(const clang::CallExpr *const callExpr) {
    identInfo_ = util::getIdentInfo(callExpr_);
    // build argument vector
    for (size_t i = 0; i < callExpr->getNumArgs(); ++i) {
        // emplace triggers ArgumentVisitor ctor
        argumentsPr_.emplace_back(callExpr->getArg(i));
    }
}

/**
 * Check if rank to enter condition is ambiguous.
 *
 * @return ambiguity
 */
bool MPIRankCase::isRankAmbiguous() const {
    // no matched condition means is else case
    if (!completeCondition_) return true;

    // if comparison op not ==, -> ambiguous
    for (const auto &rankCondition : rankConditions_) {
        for (const auto op : rankCondition.comparisonOperators_) {
            if (op->getOpcode() == BinaryOperatorKind::BO_EQ) return false;
        }
    }

    return true;
}

/**
 * Check if two ranks are rated as equal.
 *
 * @param rankCase to compare
 *
 * @return if they are equal
 */
bool MPIRankCase::isRankUnambiguouslyEqual(const MPIRankCase &rankCase) const {
    if (isRankAmbiguous() || rankCase.isRankAmbiguous()) {
        return false;
    }

    return cont::isPermutation(rankConditions_, rankCase.rankConditions_);
}

/**
 * Disect matched condition. Captures all parts seperated in containers.
 *
 * @param matchedCondition condition matched to enter case
 */
void MPIRankCase::setupConditions(const clang::Stmt *const matchedCondition) {
    if (matchedCondition) {
        completeCondition_.reset(new ConditionVisitor{matchedCondition});

        for (const auto &x : completeCondition_->comparisonOperators_) {
            // dissect single conditions, encode special mpi vars
            conditionsPr_.emplace_back(x);

            if (cont::isContained(conditionsPr_.back().valueSequence_,
                                  MPIRank::encoding)) {
                rankConditionsPr_.emplace_back(x);
            }
        }
    }
}

/**
 * Captures all MPI calls contained in case body.
 *
 * @param then
 * @param funcClassifier
 */
void MPIRankCase::setupMPICallsFromBody(
    const clang::Stmt *const then,
    const MPIFunctionClassifier &funcClassifier) {
    const CallExprVisitor callExprVisitor{then};  // collect call exprs
    for (const clang::CallExpr *const callExpr : callExprVisitor.callExprs()) {
        // add mpi calls only
        if (funcClassifier.isMPIType(util::getIdentInfo(callExpr))) {
            mpiCallsPr_.emplace_back(callExpr);
        }
    }
}

/**
 * Identify first/last ranks.
 */
void MPIRankCase::identifySpecialRanks() {
    llvm::SmallVector<std::string, 3> rankZeroA{"==", MPIRank::encoding, "0"};
    llvm::SmallVector<std::string, 3> rankZeroB{"==", "0", MPIRank::encoding};

    llvm::SmallVector<std::string, 5> rankLastA{"==", MPIRank::encoding, "-",
                                                MPIProcessCount::encoding, "1"};

    llvm::SmallVector<std::string, 5> rankLastB{
        "==", "-", MPIProcessCount::encoding, "1", MPIRank::encoding};

    for (const auto &x : rankConditions_) {
        if (x.valueSequence_ == rankZeroA || x.valueSequence_ == rankZeroB) {
            isFirstRankPr_ = true;
            break;
        }

        if (x.valueSequence_ == rankLastA || x.valueSequence_ == rankLastB) {
            isLastRankPr_ = true;
            break;
        }
    }
}

/**
 * Static function to unmark calls.
 */
void MPIRankCase::unmarkCalls() {
    for (MPIRankCase &rankCase : MPIRankCase::cases) {
        for (MPICall &call : rankCase.mpiCallsPr_) {
            call.isMarked_ = false;
        }
    }
}

}  // end of namespace: mpi
