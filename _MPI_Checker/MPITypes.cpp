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
    // at least one else case
    if (!matchedCondition_ || !rankCase.matchedCondition_) {
        if (unmatchedConditions_.size() !=
            rankCase.unmatchedConditions_.size()) {
            return false;
        } else {
            // compare unmatched conditions
            for (size_t i = 0; i < unmatchedConditions_.size(); ++i) {
                if (!unmatchedConditions_[i].isEqual(
                        rankCase.unmatchedConditions_[i], true)) {
                    return false;
                }
            }
        }
        return true;

    }
    // both cases are if/else if
    else {
        return matchedCondition_->isEqual(*(rankCase.matchedCondition_.get()),
                                          true);
    }
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
