#ifndef STMTVISITOR_HPP_9UDA2XCC
#define STMTVISITOR_HPP_9UDA2XCC

#include "clang/AST/RecursiveASTVisitor.h"

namespace mpi {

/**
 * Visitor class to traverse a statement.
 * On the way it collects binary operators, variable decls, function decls,
 * integer literals, floating literals, call expressions.
 */
class StmtVisitor : public clang::RecursiveASTVisitor<StmtVisitor> {
public:
    StmtVisitor(const clang::Stmt *const stmt) : stmt_{stmt} {
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
    bool VisitCallExpr(clang::CallExpr *);

    // non visitor functions
    enum class CompareOperators { kYes, kNo };
    bool isEqual(const StmtVisitor &visitor, CompareOperators) const;
    bool isEqualOrdered(const StmtVisitor &visitor, CompareOperators) const;
    bool isEqualPermutative(const StmtVisitor &visitor) const;
    bool containsNonCommutativeOps() const;

    // getters –––––––––––––––––––––––––––––––––––––––––––––
    const llvm::SmallVector<ComponentType, 4> &typeSequence() const {
        return typeSequence_;
    }

    const llvm::SmallVector<ComponentType, 4> &typeSequenceNoOps() const {
        return typeSequenceNoOps_;
    }

    const llvm::SmallVector<clang::BinaryOperatorKind, 1> &binaryOperators()
        const {
        return binaryOperators_;
    }

    const llvm::SmallVector<clang::VarDecl *, 1> &vars() const { return vars_; }

    const llvm::SmallVector<std::string, 1> &varNames() const {
        return varNames_;
    }

    const llvm::SmallVector<clang::FunctionDecl *, 0> &functions() const {
        return functions_;
    }

    const llvm::SmallVector<clang::IntegerLiteral *, 1> &integerLiterals()
        const {
        return integerLiterals_;
    }

    const llvm::SmallVector<clang::FloatingLiteral *, 0> &floatingLiterals()
        const {
        return floatingLiterals_;
    }

    const llvm::SmallVector<llvm::APInt, 1> &intValues() const {
        return intValues_;
    }

    const llvm::SmallVector<llvm::APFloat, 0> &floatValues() const {
        return floatValues_;
    }

    const llvm::SmallVector<clang::CallExpr *, 8> &callExprs() const {
        return callExprs_;
    }

    // complete statement
    const clang::Stmt *const stmt_;

private:
    // sequential series of component types
    llvm::SmallVector<ComponentType, 4> typeSequence_;
    // without operators
    llvm::SmallVector<ComponentType, 4> typeSequenceNoOps_;
    // extracted components
    llvm::SmallVector<clang::BinaryOperatorKind, 1> binaryOperators_;
    llvm::SmallVector<clang::VarDecl *, 1> vars_;
    llvm::SmallVector<std::string, 1> varNames_;
    llvm::SmallVector<clang::FunctionDecl *, 0> functions_;
    llvm::SmallVector<clang::IntegerLiteral *, 1> integerLiterals_;
    llvm::SmallVector<clang::FloatingLiteral *, 0> floatingLiterals_;
    llvm::SmallVector<llvm::APInt, 1> intValues_;
    llvm::SmallVector<llvm::APFloat, 0> floatValues_;

    llvm::SmallVector<clang::CallExpr *, 8> callExprs_;
};

}  // end of namespace: mpi

#endif  // end of include guard: STMTVISITOR_HPP_9UDA2XCC
