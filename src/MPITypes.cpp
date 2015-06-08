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
llvm::SmallSet<const VarDecl *, 4> visitedRankVariables;
}

llvm::SmallVector<MPIRankCase, 8> MPIRankCase::visitedRankCases;

bool MPICall::operator==(const MPICall &callToCompare) const {
    if (arguments_.size() != callToCompare.arguments_.size()) return false;
    for (size_t i = 0; i < arguments_.size(); ++i) {
        if (!arguments_[i].isEqual(callToCompare.arguments_[i])) {
            return false;
        }
    }
    return true;
}

bool MPICall::operator!=(const MPICall &callToCompare) const {
    return !(*this == callToCompare);
}

/**
 * Check if rank to enter condition is ambiguous.
 *
 * @return ambiguity
 */
bool MPIRankCase::isRankAmbiguous() const {
    // no matched condition means is else case
    if (!matchedCondition_) return true;

    // ranges used in rank conditions prohibit equality identification
    auto isRangeComparison = [](BinaryOperatorKind op) {
        return (BinaryOperatorKind::BO_LT == op ||
                BinaryOperatorKind::BO_GT == op ||
                BinaryOperatorKind::BO_LE == op ||
                BinaryOperatorKind::BO_GE == op);
    };
    for (const auto op : matchedCondition_->binaryOperators()) {
        if (isRangeComparison(op)) return true;
    }

    return false;
}

/**
 * Check if two ranks are rated as equal.
 *
 * @param rankCase to compare
 *
 * @return if they are equal
 */
bool MPIRankCase::isRankUnambiguouslyEqual(
    const MPIRankCase &rankCase) const {
    if (isRankAmbiguous() || rankCase.isRankAmbiguous()) {
        return false;
    }

    // both not ambiguous, compare matched condition
    return matchedCondition_->isEqual(*rankCase.matchedCondition_);
}

}  // end of namespace: mpi
