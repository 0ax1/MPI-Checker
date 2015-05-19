#include "MPITypes.hpp"
#include "Container.hpp"

using namespace clang;
using namespace ento;

namespace mpi {

unsigned long MPICall::id{0};

namespace MPIRank {
llvm::SmallSet<const VarDecl *, 4> visitedRankVariables;
}

llvm::SmallVector<MPIRankCase, 8> MPIRankCase::visitedRankCases;

bool MPICall::operator==(const MPICall &callToCompare) const {
    if (arguments_.size() != callToCompare.arguments_.size()) return false;
    for (size_t i = 0; i < arguments_.size(); ++i) {
        if (!arguments_[i].isEqual(callToCompare.arguments_[i],
                                   StmtVisitor::CompareOperators::kYes)) {
            return false;
        }
    }
    return true;
}

bool MPICall::operator!=(const MPICall &callToCompare) const {
    return !(*this == callToCompare);
}

bool MPIRankCase::isConditionAmbiguous() {
    if (!matchedCondition_) return true;

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
 * Check if rank cases are rated as equal.
 *
 * @param rankCase to compare
 *
 * @return if they are equal
 */
bool MPIRankCase::isConditionUnambiguouslyEqual(MPIRankCase &rankCase) {
    if (isConditionAmbiguous() || rankCase.isConditionAmbiguous()) {
        return false;
    }

    // both not ambiguos, compare matched condition
    return matchedCondition_->isEqual(*rankCase.matchedCondition_,
                                      StmtVisitor::CompareOperators::kYes);
}

}  // end of namespace: mpi
