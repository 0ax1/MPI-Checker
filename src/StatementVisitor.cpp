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

using namespace clang;
using namespace ento;

namespace mpi {

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

        // TODO handle structs, memberexpr

        varsPr_.push_back(var);
        typeSequencePr_.push_back(ComponentType::kVar);
        valueSequencePr_.push_back(var->getNameAsString());

    } else if (clang::FunctionDecl *fn =
                   clang::dyn_cast<clang::FunctionDecl>(declRef->getDecl())) {
        functionsPr_.push_back(fn);
        typeSequencePr_.push_back(ComponentType::kFunc);
        valueSequencePr_.push_back(fn->getNameAsString());
    }

    // TODO
    // clang::VarDecl *varDecl;
    // varDecl->getInitAddress();
    // clang::MemberExpr *MemberExpr;
    // MemberExpr->

    return true;
}

bool StatementVisitor::VisitMemberExpr(clang::MemberExpr *memExpr) {
    // memExpr->dumpColor();
    // TODO
    // returns field decl
    memExpr->getMemberDecl()->dumpColor();
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
    binaryOperatorsPr_.push_back(op->getOpcode());
    if (op->isComparisonOp()) {
        typeSequencePr_.push_back(ComponentType::kComparison);
    } else if (op->getOpcode() == BinaryOperatorKind::BO_Add) {
        typeSequencePr_.push_back(ComponentType::kAddOp);
    } else if (op->getOpcode() == BinaryOperatorKind::BO_Sub) {
        typeSequencePr_.push_back(ComponentType::kSubOp);
    } else {
        typeSequencePr_.push_back(ComponentType::kOperator);
    }

    valueSequencePr_.push_back(op->getOpcodeStr());

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
    integerLiteralsPr_.push_back(intLiteral);
    typeSequencePr_.push_back(ComponentType::kInt);

    SmallVector<char, 4> intValAsString;
    intLiteral->getValue().toStringUnsigned(intValAsString);
    std::string val;
    for (char c : intValAsString) {
        val.push_back(c);
    }
    valueSequencePr_.push_back(val);
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
    floatingLiteralsPr_.push_back(floatLiteral);
    typeSequencePr_.push_back(ComponentType::kFloat);

    valueSequencePr_.push_back(
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
bool StatementVisitor::isEqual(const StatementVisitor &visitorToCompare) const {
    if (containsSubtraction() || visitorToCompare.containsSubtraction()) {
        return isEqualOrdered(visitorToCompare);
    } else {
        return isEqualPermutative(visitorToCompare);
    }
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
    if (typeSequencePr_ != visitorToCompare.typeSequencePr_) return false;
    if (valueSequencePr_ != visitorToCompare.valueSequencePr_) return false;

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
    if (!cont::isPermutation(typeSequencePr_, visitorToCompare.typeSequencePr_)) {
        return false;
    }
    if (!cont::isPermutation(valueSequencePr_, visitorToCompare.valueSequencePr_)) {
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
    for (const auto binaryOperator : binaryOperatorsPr_) {
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
