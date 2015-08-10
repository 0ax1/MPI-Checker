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

#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"
#include "StatementVisitor.hpp"
#include "Container.hpp"
#include "Utility.hpp"
#include "MPITypes.hpp"

using namespace clang;
using namespace ento;

namespace mpi {

/**
 * Helper function to encode collected variables
 *
 * @param var to encode as string
 *
 * @return encoded string
 */
std::string StatementVisitor::encodeVariable(
    const clang::NamedDecl *const var) const {
    // encode rank variable
    if (cont::isContained(MPIRank::variables, var)) {
        return MPIRank::encoding;
    }
    // encode process count variable
    else if (cont::isContained(MPIProcessCount::variables, var)) {
        return MPIProcessCount::encoding;
    }
    // no special variable
    else {
        return var->getNameAsString();
    }
}

/**
 * Collect variables and functions.
 * Variables or functions can be a declrefexpr.
 *
 * @param declRef
 *
 * @return continue visiting
 */
bool StatementVisitor::VisitDeclRefExpr(clang::DeclRefExpr *declRef) {
    if (clang::VarDecl *var =
            clang::dyn_cast<clang::VarDecl>(declRef->getDecl())) {
        // only add if no struct, members are added in VisitMemberExpr
        if (var->getType()->isStructureType()) return true;

        vars_.push_back(var);
        combinedVars_.push_back(var);
        typeSequence_.push_back(ComponentType::kVar);
        valueSequence_.push_back(encodeVariable(var));

    } else if (clang::FunctionDecl *fn =
                   clang::dyn_cast<clang::FunctionDecl>(declRef->getDecl())) {
        functions_.push_back(fn);
        typeSequence_.push_back(ComponentType::kFunc);
        valueSequence_.push_back(fn->getNameAsString());
    }

    return true;
}

/**
 * Collect members from structs
 *
 * @param memExpr expression containing member
 *
 * @return continue visiting
 */
bool StatementVisitor::VisitMemberExpr(clang::MemberExpr *memExpr) {
    clang::ValueDecl *vd = memExpr->getMemberDecl();
    members_.push_back(vd);
    combinedVars_.push_back(vd);
    typeSequence_.push_back(ComponentType::kVar);  // encode as type var
    valueSequence_.push_back(encodeVariable(vd));

    return true;
}

/**
 * Collect binary operators.
 *
 * @param op
 *
 * @return continue visiting
 */
bool StatementVisitor::VisitBinaryOperator(clang::BinaryOperator *op) {
    binaryOperators_.push_back(op->getOpcode());

    if (op->isComparisonOp()) {
        typeSequence_.push_back(ComponentType::kComparison);
        comparisonOperators_.push_back(op);
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

/**
 * Collect integer literals.
 *
 * @param intLiteral
 *
 * @return continue visiting
 */
bool StatementVisitor::VisitIntegerLiteral(IntegerLiteral *intLiteral) {
    integerLiterals_.push_back(intLiteral);
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

/**
 * Collect float literals.
 *
 * @param floatLiteral
 *
 * @return continue visiting
 */
bool StatementVisitor::VisitFloatingLiteral(FloatingLiteral *floatLiteral) {
    floatingLiterals_.push_back(floatLiteral);
    typeSequence_.push_back(ComponentType::kFloat);

    valueSequence_.push_back(
        std::to_string(floatLiteral->getValueAsApproximateDouble()));
    return true;
}

/**
 * Check if components of statement are equal to compared visitor.
 * Chosen equality comparison depends on operator commutativity.
 *
 * @param visitorToCompare
 *
 * @return continue visiting
 */
bool StatementVisitor::operator==(
    const StatementVisitor &visitorToCompare) const {
    if (containsSubtraction() || visitorToCompare.containsSubtraction()) {
        return isEqualOrdered(visitorToCompare);
    } else {
        return isEqualPermutative(visitorToCompare);
    }
}

bool StatementVisitor::operator!=(
    const StatementVisitor &visitorToCompare) const {
    return !(*this == visitorToCompare);
}

/**
 * Compare operators and operands as ordered sequence.
 *
 * @param visitorToCompare
 *
 * @return equality
 */
bool StatementVisitor::isEqualOrdered(
    const StatementVisitor &visitorToCompare) const {
    if (typeSequence_ != visitorToCompare.typeSequence_) return false;
    if (valueSequence_ != visitorToCompare.valueSequence_) return false;

    return true;
}

/**
 * Compare operators and operands as permutation.
 *
 * @param visitorToCompare
 *
 * @return equality
 */
bool StatementVisitor::isEqualPermutative(
    const StatementVisitor &visitorToCompare) const {
    // type sequence must be permutation
    if (!cont::isPermutation(typeSequence_,
                             visitorToCompare.typeSequence_)) {
        return false;
    }
    if (!cont::isPermutation(valueSequence_,
                             visitorToCompare.valueSequence_)) {
        return false;
    }

    return true;
}

/**
 * Check if statement contains a subtraction.
 *
 * @return is inverse
 */
bool StatementVisitor::containsSubtraction() const {
    for (const auto binaryOperator : binaryOperators_) {
        if (binaryOperator == BinaryOperatorKind::BO_Sub) {
            return true;
        }
    }
    return false;
}

/**
 * Check if the last operator is "inverse".
 *
 * @param visitor
 *
 * @return is inverse
 */
bool StatementVisitor::isLastOperatorInverse(
    const StatementVisitor &visitor) const {
    // last operator must be inverse
    return (BinaryOperatorKind::BO_Add == binaryOperators_.front() &&
            BinaryOperatorKind::BO_Sub == visitor.binaryOperators_.front()) ||

           (BinaryOperatorKind::BO_Sub == binaryOperators_.front() &&
            BinaryOperatorKind::BO_Add == visitor.binaryOperators_.front());
}

}  // end of namespace: mpi
