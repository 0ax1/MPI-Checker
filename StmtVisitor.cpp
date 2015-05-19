#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"
#include "StmtVisitor.hpp"
#include "Container.hpp"
#include "Utility.hpp"

using namespace clang;
using namespace ento;

namespace mpi {

// variables or functions can be a declrefexpr
bool StmtVisitor::VisitDeclRefExpr(clang::DeclRefExpr *declRef) {
    if (clang::VarDecl *var =
            clang::dyn_cast<clang::VarDecl>(declRef->getDecl())) {
        vars_.push_back(var);
        typeSequence_.push_back(ComponentType::kVar);
        valueSequence_.push_back(var->getNameAsString());
    } else if (clang::FunctionDecl *fn =
                   clang::dyn_cast<clang::FunctionDecl>(declRef->getDecl())) {
        functions_.push_back(fn);
        typeSequence_.push_back(ComponentType::kFunc);
        valueSequence_.push_back(fn->getNameAsString());
    }
    return true;
}

bool StmtVisitor::VisitBinaryOperator(clang::BinaryOperator *op) {
    binaryOperators_.push_back(op->getOpcode());
    if (op->isComparisonOp()) {
        typeSequence_.push_back(ComponentType::kComparsison);
    } else if (op->getOpcode() == BinaryOperatorKind::BO_Add) {
        typeSequence_.push_back(ComponentType::kAddOp);
    } else if (op->getOpcode() == BinaryOperatorKind::BO_Sub) {
        typeSequence_.push_back(ComponentType::kSubOp);
    } else {
        typeSequence_.push_back(ComponentType::kOperator);
    }

    valueSequence_.push_back(op->getOpcodeStr());

    return true;
}

bool StmtVisitor::VisitIntegerLiteral(IntegerLiteral *intLiteral) {
    integerLiterals_.push_back(intLiteral);
    intValues_.push_back(intLiteral->getValue());
    typeSequence_.push_back(ComponentType::kInt);

    SmallVector<char, 4> intValAsString;
    intLiteral->getValue().toStringUnsigned(intValAsString);
    std::string val;
    for (char c : intValAsString) {
        val.push_back(c);
    }
    valueSequence_.push_back(val);
    return true;
}

bool StmtVisitor::VisitFloatingLiteral(FloatingLiteral *floatLiteral) {
    floatingLiterals_.push_back(floatLiteral);
    floatValues_.push_back(floatLiteral->getValue());
    typeSequence_.push_back(ComponentType::kFloat);

    valueSequence_.push_back(
        std::to_string(floatLiteral->getValueAsApproximateDouble()));
    return true;
}

bool StmtVisitor::isEqual(const StmtVisitor &visitorToCompare) const {
    if (containsMinus() || visitorToCompare.containsMinus()) {
        return isEqualOrdered(visitorToCompare);
    } else {
        return isEqualPermutative(visitorToCompare);
    }
}

bool StmtVisitor::isEqualOrdered(const StmtVisitor &visitorToCompare) const {
    if (typeSequence_ != visitorToCompare.typeSequence_) return false;
    if (valueSequence_ != visitorToCompare.valueSequence_) return false;

    return true;
}

bool StmtVisitor::isEqualPermutative(
    const StmtVisitor &visitorToCompare) const {
    // type sequence must be permutation
    if (!cont::isPermutation(typeSequence_, visitorToCompare.typeSequence_)) {
        return false;
    }
    if (!cont::isPermutation(valueSequence_, visitorToCompare.valueSequence_)) {
        return false;
    }

    return true;
}

bool StmtVisitor::containsMinus() const {
    for (const auto binaryOperator : binaryOperators_) {
        if (binaryOperator == BinaryOperatorKind::BO_Sub) {
            return true;
        }
    }
    return false;
}

bool StmtVisitor::isLastOperatorInverse(const StmtVisitor &visitor) const {
    // last operator must be inverse
    return (BinaryOperatorKind::BO_Add == binaryOperators_.front() &&
            BinaryOperatorKind::BO_Sub == visitor.binaryOperators().front()) ||

           (BinaryOperatorKind::BO_Sub == binaryOperators_.front() &&
            BinaryOperatorKind::BO_Add == visitor.binaryOperators().front());
}

}  // end of namespace: mpi
