#include "MPITypes.hpp"
#include "Container.hpp"

namespace mpi {

llvm::SmallVector<MPICall, 16> MPICall::visitedCalls;
unsigned long MPICall::id{0};

namespace MPIRank {
llvm::SmallSet<const clang::VarDecl *, 4> visitedRankVariables;
}

llvm::SmallVector<MPIRankCase, 8> MPIRankCase::visitedRankCases;

/**
 * Check if rank cases are rated as equal.
 * If one case is not in standard form
 * the comparison rates them as distinct.
 *
 * @param rankCase to compare
 *
 * @return if they are equal
 */
bool MPIRankCase::isRankConditionEqual(MPIRankCase &rankCase) {
    return (isConditionTypeStandard() && rankCase.isConditionTypeStandard() &&
            matchedCondition_->intValues_.front() ==
                rankCase.matchedCondition_->intValues_.front());
}

/**
 * Sets case condition type depending on standard form.
 * Standard conditions must have the form (rank == intLiteral).
 */
void MPIRankCase::initConditionType() {
    isConditionTypeStandard_ =
        // only one int literal
        (matchedCondition_->integerLiterals_.size() == 1 &&

         // only variable is rank variable
         matchedCondition_->vars_.size() == 1 &&
         cont::isContained(MPIRank::visitedRankVariables,
                           matchedCondition_->vars_.front()) &&

         // only operator is == operator
         matchedCondition_->binaryOperators_.size() == 1 &&
         matchedCondition_->binaryOperators_.front() ==
             clang::BinaryOperatorKind::BO_EQ &&

         // no floats
         matchedCondition_->floatingLiterals_.size() == 0 &&

         // no functions
         matchedCondition_->functions_.size() == 0);
}

bool MPIRankCase::isConditionTypeStandard() { return isConditionTypeStandard_; }

bool RankVisitor::VisitCallExpr(clang::CallExpr *callExpr) {
    MPICall mpiCall{callExpr};
    if (funcClassifier_.isMPI_Comm_rank(mpiCall)) {
        clang::VarDecl *varDecl = mpiCall.arguments_[1].vars_[0];
        MPIRank::visitedRankVariables.insert(varDecl);
    }

    return true;
}

}  // end of namespace: mpi
