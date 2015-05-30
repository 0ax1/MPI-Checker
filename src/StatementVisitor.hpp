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

#ifndef STATEMENTVISITOR_HPP_9UDA2XCC
#define STATEMENTVISITOR_HPP_9UDA2XCC

#include "clang/AST/RecursiveASTVisitor.h"

namespace mpi {

/**
 * Visitor class to traverse a statement.
 * On the way it collects binary operators, variable decls, function decls,
 * integer literals, floating literals.
 */
class StatementVisitor : public clang::RecursiveASTVisitor<StatementVisitor> {
public:
    StatementVisitor(const clang::Stmt *const stmt) : stmt_{stmt} {
        TraverseStmt(const_cast<clang::Stmt *>(stmt_));
    }

    enum class ComponentType {
        kInt,
        kFloat,
        kVar,
        kFunc,
        kComparsison,
        kAddOp,
        kSubOp,
        kOperator
    };

    // must be public to trigger callbacks
    bool VisitDeclRefExpr(clang::DeclRefExpr *);
    bool VisitBinaryOperator(clang::BinaryOperator *);
    bool VisitIntegerLiteral(clang::IntegerLiteral *);
    bool VisitFloatingLiteral(clang::FloatingLiteral *);

    // non visitor functions
    bool isEqual(const StatementVisitor &) const;
    bool isEqualOrdered(const StatementVisitor &) const;
    bool isEqualPermutative(const StatementVisitor &) const;
    bool containsSubtraction() const;
    bool isLastOperatorInverse(const StatementVisitor &) const;

    // getters –––––––––––––––––––––––––––––––––––––––––––––
    const llvm::SmallVectorImpl<ComponentType> &typeSequence() const {
        return typeSequence_;
    }

    const llvm::SmallVectorImpl<clang::BinaryOperatorKind> &binaryOperators()
        const {
        return binaryOperators_;
    }

    const llvm::SmallVectorImpl<clang::VarDecl *> &vars() const {
        return vars_;
    }

    const llvm::SmallVectorImpl<clang::FunctionDecl *> &functions() const {
        return functions_;
    }

    const llvm::SmallVectorImpl<clang::IntegerLiteral *> &integerLiterals()
        const {
        return integerLiterals_;
    }

    const llvm::SmallVectorImpl<clang::FloatingLiteral *> &floatingLiterals()
        const {
        return floatingLiterals_;
    }

    const llvm::SmallVectorImpl<std::string> &valueSequence() const {
        return valueSequence_;
    }

    // complete statement
    const clang::Stmt *const stmt_;

private:
    // sequential series of types
    llvm::SmallVector<ComponentType, 4> typeSequence_;
    // sequential series of values
    llvm::SmallVector<std::string, 4> valueSequence_;
    // components
    llvm::SmallVector<clang::BinaryOperatorKind, 1> binaryOperators_;
    llvm::SmallVector<clang::VarDecl *, 1> vars_;
    llvm::SmallVector<clang::FunctionDecl *, 0> functions_;
    llvm::SmallVector<clang::IntegerLiteral *, 1> integerLiterals_;
    llvm::SmallVector<clang::FloatingLiteral *, 0> floatingLiterals_;
};

// aliases
using ArgumentVisitor = StatementVisitor;
using ConditionVisitor = StatementVisitor;

}  // end of namespace: mpi

#endif  // end of include guard: STATEMENTVISITOR_HPP_9UDA2XCC
