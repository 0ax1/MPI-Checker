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
        declarations_.push_back(var);
        sequentialSeries_.push_back(declRef);
    } else if (clang::FunctionDecl *fn =
                   clang::dyn_cast<clang::FunctionDecl>(declRef->getDecl())) {
        functions_.push_back(fn);
        declarations_.push_back(fn);
        sequentialSeries_.push_back(declRef);
    }
    return true;
}

bool StmtVisitor::VisitBinaryOperator(clang::BinaryOperator *op) {
    binaryOperators_.push_back(op->getOpcode());
    sequentialSeries_.push_back(op);
    return true;
}

bool StmtVisitor::VisitIntegerLiteral(IntegerLiteral *intLiteral) {
    integerLiterals_.push_back(intLiteral);
    intValues_.push_back(intLiteral->getValue());
    return true;
}

bool StmtVisitor::VisitFloatingLiteral(FloatingLiteral *floatLiteral) {
    floatingLiterals_.push_back(floatLiteral);
    floatValues_.push_back(floatLiteral->getValue());
    return true;
}

bool StmtVisitor::VisitCallExpr(clang::CallExpr *callExpr) {
    callExprs_.push_back(callExpr);
    return true;
}

bool StmtVisitor::areContainerSizesEqual(const StmtVisitor &visitorToCompare) const {
    // count of all elements must match
    if (sequentialSeries_.size() != visitorToCompare.sequentialSeries_.size()) {
        return false;
    }

    // int literals
    if (intValues_ != visitorToCompare.intValues_) return false;

    // float literals (just compare size, not by value)
    if (floatingLiterals_.size() != visitorToCompare.floatingLiterals_.size())
        return false;

    // functions
    if (functions_ != visitorToCompare.functions_) return false;

    if (vars_.size() != visitorToCompare.vars_.size()) return false;

    return true;
}

bool StmtVisitor::isEqualOrdered(const StmtVisitor &visitorToCompare,
                                 bool compareOperators) const {

    if (!areContainerSizesEqual(visitorToCompare)) return false;

    // compare classes in sequence
    for (size_t i = 0; i < sequentialSeries_.size(); ++i) {
        if (!(sequentialSeries_[i]->getStmtClass() ==
              visitorToCompare.sequentialSeries_[i]->getStmtClass())) {
            return false;
        }
    }

    // compare decl types in sequence
    for (size_t i = 0; i < declarations_.size(); ++i) {
        if (!(isa<VarDecl>(declarations_[i]) ==
                  isa<VarDecl>(visitorToCompare.declarations_[i]) &&
              isa<FunctionDecl>(declarations_[i]) ==
                  isa<FunctionDecl>(visitorToCompare.declarations_[i]))) {
            return false;
        }
    }


    // variables (are compared by name, to make them comparable
    // beyond their scope, across different branches, functions)
    for (size_t i = 0; i < vars_.size(); ++i) {
        if (vars_[i]->getNameAsString() !=
            visitorToCompare.vars_[i]->getNameAsString()) {
            // if name not equal check if they are both rank vars
            bool isRankVar1 =
                cont::isContained(MPIRank::visitedRankVariables, vars_[i]);

            bool isRankVar2 = cont::isContained(MPIRank::visitedRankVariables,
                                                visitorToCompare.vars_[i]);
            if (isRankVar1 && isRankVar2) continue;

            return false;
        }
    }

    // operators
    if (compareOperators) {
        if (binaryOperators_ != visitorToCompare.binaryOperators_) return false;
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

}  // end of namespace: mpi
