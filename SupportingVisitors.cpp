#include <string>
#include "SupportingVisitors.hpp"
#include "Container.hpp"
#include "MPITypes.hpp"

using namespace clang;
using namespace ento;

namespace mpi {

// variables or functions can be a declrefexpr
bool StmtVisitor::VisitDeclRefExpr(clang::DeclRefExpr *declRef) {
    if (clang::VarDecl *var =
            clang::dyn_cast<clang::VarDecl>(declRef->getDecl())) {
        vars_.push_back(var);
        varNames_.push_back(var->getNameAsString());
        typeSequence_.push_back(ComponentType::kVar);
        typeSequenceNoOps_.push_back(ComponentType::kVar);
    } else if (clang::FunctionDecl *fn =
                   clang::dyn_cast<clang::FunctionDecl>(declRef->getDecl())) {
        functions_.push_back(fn);
        typeSequence_.push_back(ComponentType::kFunc);
        typeSequenceNoOps_.push_back(ComponentType::kFunc);
    }
    return true;
}

bool StmtVisitor::VisitBinaryOperator(clang::BinaryOperator *op) {
    binaryOperators_.push_back(op->getOpcode());
    if (op->isComparisonOp()) {
        typeSequence_.push_back(ComponentType::kComparsison);
    } else if (op->getOpcode() == BinaryOperatorKind::BO_Add) {
        typeSequence_.push_back(ComponentType::kAddOp);
    } else if (op->getOpcode() == BinaryOperatorKind::BO_Add) {
        typeSequence_.push_back(ComponentType::kSubOp);
    } else {
        typeSequence_.push_back(ComponentType::kOperator);
    }
    return true;
}

bool StmtVisitor::VisitIntegerLiteral(IntegerLiteral *intLiteral) {
    integerLiterals_.push_back(intLiteral);
    intValues_.push_back(intLiteral->getValue());
    typeSequence_.push_back(ComponentType::kInt);
    typeSequenceNoOps_.push_back(ComponentType::kInt);
    return true;
}

bool StmtVisitor::VisitFloatingLiteral(FloatingLiteral *floatLiteral) {
    floatingLiterals_.push_back(floatLiteral);
    floatValues_.push_back(floatLiteral->getValue());
    typeSequence_.push_back(ComponentType::kFloat);
    typeSequenceNoOps_.push_back(ComponentType::kFloat);
    return true;
}

bool StmtVisitor::VisitCallExpr(clang::CallExpr *callExpr) {
    callExprs_.push_back(callExpr);
    return true;
}

bool StmtVisitor::isEqual(const StmtVisitor &visitorToCompare,
                          CompareOperators compareOperators) const {
    if (containsNonCommutativeOps() ||
        visitorToCompare.containsNonCommutativeOps()) {
        return isEqualOrdered(visitorToCompare, compareOperators);
    } else {
        return isEqualPermutative(visitorToCompare);
    }
}

bool StmtVisitor::isEqualOrdered(const StmtVisitor &visitorToCompare,
                                 CompareOperators compareOperators) const {
    // include operator comparison
    if (compareOperators == CompareOperators::kYes) {
        // complete type sequence must match
        if (typeSequence_ != visitorToCompare.typeSequence_) return false;
    }
    // omit operator comparison
    else {
        // type sequence without operators
        if (typeSequenceNoOps_ != visitorToCompare.typeSequenceNoOps_)
            return false;
    }

    // int literals
    if (intValues_ != visitorToCompare.intValues_) return false;
    // functions
    if (functions_ != visitorToCompare.functions_) return false;
    // variables
    if (varNames_ != visitorToCompare.varNames_) return false;
    // operators
    if (compareOperators == CompareOperators::kYes) {
        if (binaryOperators_ != visitorToCompare.binaryOperators_) return false;
    }

    return true;
}

bool StmtVisitor::isEqualPermutative(
    const StmtVisitor &visitorToCompare) const {
    // type sequence must be permutation
    if (!cont::isPermutation(typeSequence_, visitorToCompare.typeSequence_)) {
        return false;
    }

    // operators
    if (!cont::isPermutation(binaryOperators_,
                             visitorToCompare.binaryOperators_))
        return false;
    // variables (are compared by name, to make them comparable
    // beyond their scope, across different branches, functions)
    if (!cont::isPermutation(varNames_, visitorToCompare.varNames_))
        return false;

    // int literals
    if (!cont::isPermutation(intValues_, visitorToCompare.intValues_))
        return false;

    // functions
    if (!cont::isPermutation(functions_, visitorToCompare.functions_))
        return false;

    return true;
}

bool StmtVisitor::containsNonCommutativeOps() const {
    for (const auto binaryOperator : binaryOperators_) {
        if (binaryOperator == BinaryOperatorKind::BO_Sub) {
            return true;
        }
    }
    return false;
}

bool ArrayVisitor::VisitDeclRefExpr(clang::DeclRefExpr *declRef) {
    if (clang::VarDecl *var =
            clang::dyn_cast<clang::VarDecl>(declRef->getDecl())) {
        vars_.push_back(var);
    }
    return true;
}

bool RankVisitor::VisitCallExpr(CallExpr *callExpr) {
    if (funcClassifier_.isMPIType(callExpr->getDirectCallee()->getIdentifier())) {
        MPICall mpiCall{callExpr};
        if (funcClassifier_.isMPI_Comm_rank(mpiCall)) {
            VarDecl *varDecl = mpiCall.arguments_[1].vars_[0];
            MPIRank::visitedRankVariables.insert(varDecl);
        }
    }

    return true;
}

}  // end of namespace: mpi
