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
// #include "clang/ASTMatchers/ASTMatchers.h"

using namespace clang;
using namespace ento;

namespace mpi {

unsigned long MPICall::idCounter{0};

namespace MPIRank {
llvm::SmallSet<const ValueDecl *, 4> variables;
}

namespace MPIProcessCount {
llvm::SmallSet<const ValueDecl *, 4> variables;
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

}  // end of namespace: mpi
