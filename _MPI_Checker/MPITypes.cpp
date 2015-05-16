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

bool StmtVisitor::isEqualOrdered(const StmtVisitor &visitorToCompare,
                                 bool compareOperators) const {
    // count of all elements must match
    if (sequentialSeries_.size() != visitorToCompare.sequentialSeries_.size()) {
        return false;
    }

    // compare classes in sequence
    for (size_t i = 0; i < sequentialSeries_.size(); ++i) {
        if (!(sequentialSeries_[i]->getStmtClass() ==
              visitorToCompare.sequentialSeries_[i]->getStmtClass())) {
            return false;
        }
    }

    // compare decl types in sequence
    for (size_t i = 0; i < sequentialSeries_.size(); ++i) {
        if (!(declarations_[i]->classofKind(Decl::Kind::Function) ==
                  visitorToCompare.declarations_[i]->classofKind(
                      Decl::Kind::Function) &&

              declarations_[i]->classofKind(Decl::Kind::Var) ==
                  visitorToCompare.declarations_[i]->classofKind(
                      Decl::Kind::Var))) {
            return false;
        }
    }

    // int literals
    if (intValues_ != visitorToCompare.intValues_) return false;

    // float literals (just compare size, not by value)
    if (floatingLiterals_.size() != visitorToCompare.floatingLiterals_.size())
        return false;

    // functions
    if (functions_ != visitorToCompare.functions_) return false;

    // operators
    if (compareOperators) {
        if (binaryOperators_ != visitorToCompare.binaryOperators_) return false;
    }

    // variables (are compared by name, to make them comparable
    // beyond their scope, across different branches, functions)
    if (vars_.size() != visitorToCompare.vars_.size()) return false;

    for (size_t i = 0; i < vars_.size(); ++i) {
        if (vars_[i]->getNameAsString() !=
            visitorToCompare.vars_[i]->getNameAsString()) {

            bool isRankVar1 =
                cont::isContained(MPIRank::visitedRankVariables, vars_[i]);

            bool isRankVar2 = cont::isContained(MPIRank::visitedRankVariables,
                                                visitorToCompare.vars_[i]);
            // ok if both are rank vars
            if (!(isRankVar1 && isRankVar2)) continue;

            return false;
        }
    }

    return true;
}

bool StmtVisitor::isEqualPermutative(const StmtVisitor &visitorToCompare,
                                     bool compareOperators) const {
    // count of all elements must match
    if (sequentialSeries_.size() != visitorToCompare.sequentialSeries_.size()) {
        return false;
    }

    // operators
    if (compareOperators) {
        if (binaryOperators_ != visitorToCompare.binaryOperators_) return false;
    }

    // variables (are compared by name, to make them comparable
    // beyond their scope, across different branches, functions)
    if (vars_.size() != visitorToCompare.vars_.size()) return false;
    llvm::SmallVector<std::string, 2> varNames1;
    llvm::SmallVector<std::string, 2> varNames2;
    for (size_t i = 0; i < vars_.size(); ++i) {
        varNames1.push_back(vars_[i]->getNameAsString());
        varNames2.push_back(visitorToCompare.vars_[i]->getNameAsString());
    }
    if (!cont::isPermutation(varNames1, varNames2)) return false;

    // int literals
    if (!cont::isPermutation(intValues_, visitorToCompare.intValues_))
        return false;

    // float literals (just compare size, not by value)
    if (floatingLiterals_.size() != visitorToCompare.floatingLiterals_.size())
        return false;

    // functions
    if (!cont::isPermutation(functions_, visitorToCompare.functions_))
        return false;

    return true;
}

bool StmtVisitor::containsNonCommutativeOps() const {
    for (const auto binaryOperator : binaryOperators_) {
        if (binaryOperator != BinaryOperatorKind::BO_Add &&
            binaryOperator != BinaryOperatorKind::BO_Mul)
            return false;
    }
    return true;
}

/**
 * Check if rank cases are rated as equal.
 *
 * @param rankCase to compare
 *
 * @return if they are equal
 */
bool MPIRankCase::isRankConditionEqual(MPIRankCase &rankCase) {
    if (isConditionTypeStandard() && rankCase.isConditionTypeStandard()) {
        return (matchedCondition_->intValues_.front() ==
                rankCase.matchedCondition_->intValues_.front());
    } else {
        if (matchedCondition_->containsNonCommutativeOps()) {
            return matchedCondition_->isEqualOrdered(
                *(rankCase.matchedCondition_.get()), true);
        } else {
            return matchedCondition_->isEqualPermutative(
                *(rankCase.matchedCondition_.get()), true);
        }
    }
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
             BinaryOperatorKind::BO_EQ &&

         // no floats
         matchedCondition_->floatingLiterals_.size() == 0 &&

         // no functions
         matchedCondition_->functions_.size() == 0);
}

bool MPIRankCase::isConditionTypeStandard() { return isConditionTypeStandard_; }

bool RankVisitor::VisitCallExpr(CallExpr *callExpr) {
    MPICall mpiCall{callExpr};
    if (funcClassifier_.isMPI_Comm_rank(mpiCall)) {
        VarDecl *varDecl = mpiCall.arguments_[1].vars_[0];
        MPIRank::visitedRankVariables.insert(varDecl);
    }

    return true;
}

}  // end of namespace: mpi
