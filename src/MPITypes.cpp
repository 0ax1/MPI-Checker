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
llvm::SmallSet<const Decl *, 4> variables;
}

namespace MPIProcessCount {
llvm::SmallSet<const Decl *, 4> variables;
}

llvm::SmallVector<MPIRankCase, 8> MPIRankCase::cases;

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

    // TODO extract comparison for rank
    // -> rest is not relevant
    // if binOp lhs, rhs is rank var -> save bin op
    // hard..
    // multiple rank case vars in one condition... nicht wirklich

    // clang::ast_matchers::DeclarationMatcher funcDecl =
    // clang::ast_matchers::functionDecl().bind("func");

    // no matched condition means is else case
    if (!matchedCondition_) return true;

    // TODO make ambiguity criteria more precise
    // rank == 1 && x > 0 would be rated as ambiguous
    // rating must be bound to rank var
    // condition must be changed everything not being == is ambiguous
    // write down rank if concrete (also as expression
    // rank = processcount - 1
    // const for last rank
    // track, identify process count variables

    // ranges used in rank conditions prohibit equality identification
    auto isRangeComparison = [](BinaryOperatorKind op) {
        return (BinaryOperatorKind::BO_LT == op ||
                BinaryOperatorKind::BO_GT == op ||
                BinaryOperatorKind::BO_LE == op ||
                BinaryOperatorKind::BO_GE == op);
    };
    for (const auto op : matchedCondition_->binaryOperators_) {
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
bool MPIRankCase::isRankUnambiguouslyEqual(const MPIRankCase &rankCase) const {
    if (isRankAmbiguous() || rankCase.isRankAmbiguous()) {
        return false;
    }

    // TODO just compare ranks
    // both not ambiguous, compare matched condition
    return matchedCondition_->isEqual(*rankCase.matchedCondition_);
}

}  // end of namespace: mpi
