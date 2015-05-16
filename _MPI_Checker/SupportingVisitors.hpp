#ifndef SUPPORTINGVISITORS_HPP_NWUC3OWQ
#define SUPPORTINGVISITORS_HPP_NWUC3OWQ

#include "../ClangSACheckers.h"
#include "clang/StaticAnalyzer/Core/Checker.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "MPIFunctionClassifier.hpp"

namespace mpi {

/**
 * Visitor class to traverse a statement.
 * On the way it collects binary operators, variable decls, function decls,
 * integer literals, floating literals, call expressions.
 */
class StmtVisitor : public clang::RecursiveASTVisitor<StmtVisitor> {
public:
    StmtVisitor(clang::Stmt *stmt) : stmt_{stmt} { TraverseStmt(stmt_); }

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
    bool areDeclTypesEqual(const StmtVisitor &) const;
    bool areVariablesEqual(const StmtVisitor &) const;

    // complete statement
    clang::Stmt *stmt_;

    // sequential series of components
    llvm::SmallVector<clang::Stmt *, 4> sequentialSeries_;
    llvm::SmallVector<clang::Decl *, 2> declarations_;

    // extracted components
    llvm::SmallVector<clang::BinaryOperatorKind, 1> binaryOperators_;
    llvm::SmallVector<clang::VarDecl *, 1> vars_;
    llvm::SmallVector<clang::FunctionDecl *, 0> functions_;
    llvm::SmallVector<clang::IntegerLiteral *, 1> integerLiterals_;
    llvm::SmallVector<clang::FloatingLiteral *, 0> floatingLiterals_;
    llvm::SmallVector<llvm::APInt, 1> intValues_;
    llvm::SmallVector<llvm::APFloat, 0> floatValues_;

    llvm::SmallVector<clang::CallExpr *, 8> callExprs_;
};

class ArrayVisitor : public clang::RecursiveASTVisitor<ArrayVisitor> {
public:
    ArrayVisitor(clang::VarDecl *varDecl) : arrayVarDecl_{varDecl} {
        TraverseVarDecl(arrayVarDecl_);
    }
    // must be public to trigger callbacks
    bool VisitDeclRefExpr(clang::DeclRefExpr *);

    // complete VarDecl expression
    clang::VarDecl *arrayVarDecl_;
    // extracted components
    llvm::SmallVector<clang::VarDecl *, 4> vars_;
};

/**
 * Class to find out type information for a given qualified type.
 * Detects if a QualType is a typedef, complex type, builtin type.
 * For matches the type information is stored.
 */
class TypeVisitor : public clang::RecursiveASTVisitor<TypeVisitor> {
public:
    TypeVisitor(clang::QualType qualType) { TraverseType(qualType); }

    // must be public to trigger callbacks
    bool VisitTypedefType(clang::TypedefType *tdt) {
        typedefTypeName_ = tdt->getDecl()->getQualifiedNameAsString();
        isTypedefType_ = true;
        return true;
    }

    bool VisitBuiltinType(clang::BuiltinType *builtinType) {
        builtinType_ = builtinType;
        return true;
    }

    bool VisitComplexType(clang::ComplexType *complexType) {
        complexType_ = complexType;
        return true;
    }

    // passed qual type
    clang::QualType *qualType_;

    bool isTypedefType_{false};
    std::string typedefTypeName_;

    clang::BuiltinType *builtinType_{nullptr};
    clang::ComplexType *complexType_{nullptr};
};

/**
 * Visitor class to collect rank variables.
 */
class RankVisitor : public clang::RecursiveASTVisitor<RankVisitor> {
public:
    RankVisitor(clang::ento::AnalysisManager &analysisManager)
        : funcClassifier_{analysisManager} {}

    // collect rank vars
    bool VisitCallExpr(clang::CallExpr *);

private:
    MPIFunctionClassifier funcClassifier_;
};

}  // end of namespace: mpi

#endif  // end of include guard: SUPPORTINGVISITORS_HPP_NWUC3OWQ
