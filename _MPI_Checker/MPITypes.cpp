#include "MPITypes.hpp"
#include "Container.hpp"

using namespace clang;
using namespace ento;

namespace mpi {

llvm::SmallVector<MPICall, 16> MPICall::visitedCalls;
unsigned long MPICall::id{0};

namespace MPIRank {
llvm::SmallSet<const VarDecl *, 4> visitedRankVariables;
}

llvm::SmallVector<MPIRankCase, 8> MPIRankCase::visitedRankCases;

/**
 * Check if rank cases are rated as equal.
 *
 * @param rankCase to compare
 *
 * @return if they are equal
 */
bool MPIRankCase::isRankConditionEqual(MPIRankCase &rankCase) {
    // else cases prohibit equality identification
    if ((!matchedCondition_ || !rankCase.matchedCondition_)) {
        return false;
    }

    // ranges used in rank conditions prohibit equality identification
    auto isRangeComparison = [](BinaryOperatorKind op) {
        return (BinaryOperatorKind::BO_LT == op ||
            BinaryOperatorKind::BO_GT == op ||
            BinaryOperatorKind::BO_LE == op || BinaryOperatorKind::BO_GE == op);
    };
    for (const auto op : matchedCondition_->binaryOperators_) {
        if (isRangeComparison(op)) return false;
    }
    for (const auto op : rankCase.matchedCondition_->binaryOperators_) {
        if (isRangeComparison(op)) return false;
    }

    // both cases are if/else if
    // compare matched condition
    return matchedCondition_->isEqual(*rankCase.matchedCondition_, true);
}

bool RankVisitor::VisitCallExpr(CallExpr *callExpr) {
    MPICall mpiCall{callExpr};
    if (funcClassifier_.isMPI_Comm_rank(mpiCall)) {
        VarDecl *varDecl = mpiCall.arguments_[1].vars_[0];
        MPIRank::visitedRankVariables.insert(varDecl);
    }

    return true;
}

}  // end of namespace: mpi
