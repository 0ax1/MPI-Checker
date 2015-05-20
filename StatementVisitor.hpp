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
    bool containsMinus() const;
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
