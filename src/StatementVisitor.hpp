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
    StatementVisitor(const clang::Stmt *const stmt)
        : stmt_{stmt},
          typeSequence_{typeSequencePr_},
          valueSequence_{valueSequencePr_},
          binaryOperators_{binaryOperatorsPr_},
          vars_{varsPr_},
          members_{membersPr_},
          functions_{functionsPr_},
          integerLiterals_{integerLiteralsPr_},
          floatingLiterals_{floatingLiteralsPr_} {
        TraverseStmt(const_cast<clang::Stmt *>(stmt_));
    }

    StatementVisitor(const StatementVisitor &stmtVisitor)
        : stmt_{stmtVisitor.stmt_},
          typeSequence_{typeSequencePr_},
          valueSequence_{valueSequencePr_},
          binaryOperators_{binaryOperatorsPr_},
          vars_{varsPr_},
          members_{membersPr_},
          functions_{functionsPr_},
          integerLiterals_{integerLiteralsPr_},
          floatingLiterals_{floatingLiteralsPr_} {
        TraverseStmt(const_cast<clang::Stmt *>(stmt_));
    }

    enum class ComponentType {
        kInt,
        kFloat,
        kVar,
        kFunc,
        kComparison,
        kAddOp,
        kSubOp,
        kOperator
    };

    // must be public to trigger callbacks
    bool VisitDeclRefExpr(clang::DeclRefExpr *);
    bool VisitMemberExpr(clang::MemberExpr *);
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
    // references to private members
    const clang::Stmt *const stmt_;

    const llvm::SmallVector<ComponentType, 4> &typeSequence_;
    const llvm::SmallVector<std::string, 4> &valueSequence_;

    const llvm::SmallVector<clang::BinaryOperatorKind, 1> &binaryOperators_;
    const llvm::SmallVector<clang::VarDecl *, 1> &vars_;
    const llvm::SmallVector<clang::ValueDecl *, 1> &members_;  // member vars
    const llvm::SmallVector<clang::FunctionDecl *, 0> &functions_;
    const llvm::SmallVector<clang::IntegerLiteral *, 1> &integerLiterals_;
    const llvm::SmallVector<clang::FloatingLiteral *, 0> &floatingLiterals_;

private:
    std::string encodeVariable(const clang::NamedDecl *const);

    // sequential series of types
    llvm::SmallVector<ComponentType, 4> typeSequencePr_;
    // sequential series of values
    llvm::SmallVector<std::string, 4> valueSequencePr_;
    // components
    llvm::SmallVector<clang::BinaryOperatorKind, 1> binaryOperatorsPr_;
    llvm::SmallVector<clang::VarDecl *, 1> varsPr_;
    llvm::SmallVector<clang::ValueDecl *, 1> membersPr_;  // member vars
    llvm::SmallVector<clang::FunctionDecl *, 0> functionsPr_;
    llvm::SmallVector<clang::IntegerLiteral *, 1> integerLiteralsPr_;
    llvm::SmallVector<clang::FloatingLiteral *, 0> floatingLiteralsPr_;
};

// aliases
using ArgumentVisitor = StatementVisitor;
using ConditionVisitor = StatementVisitor;

}  // end of namespace: mpi

#endif  // end of include guard: STATEMENTVISITOR_HPP_9UDA2XCC
