#ifndef SUPPORTINGVISITORS_HPP_NWUC3OWQ
#define SUPPORTINGVISITORS_HPP_NWUC3OWQ

#include "../ClangSACheckers.h"
#include "clang/StaticAnalyzer/Core/Checker.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"
#include "clang/AST/RecursiveASTVisitor.h"

#include "Typedefs.hpp"

namespace mpi {

/**
 * Visitor class to traverse a call-expression argument.
 * On the way it collects binary operators, variable decls, function decls,
 * integer literals, floating literals.
 */
class SingleArgVisitor : public clang::RecursiveASTVisitor<SingleArgVisitor> {
public:
    SingleArgVisitor(clang::CallExpr *argExpression, size_t idx)
        : expr_{argExpression->getArg(idx)} {
        TraverseStmt(expr_);
    }

    // must be public to trigger callbacks
    bool VisitDeclRefExpr(clang::DeclRefExpr *);
    bool VisitBinaryOperator(clang::BinaryOperator *);
    bool VisitIntegerLiteral(clang::IntegerLiteral *);
    bool VisitFloatingLiteral(clang::FloatingLiteral *);

    // complete argument expression
    clang::Expr *expr_;
    // extracted components
    llvm::SmallVector<clang::BinaryOperatorKind, 1> binaryOperators_;
    llvm::SmallVector<clang::VarDecl *, 1> vars_;
    llvm::SmallVector<clang::FunctionDecl *, 0> functions_;
    llvm::SmallVector<clang::IntegerLiteral *, 1> integerLiterals_;
    llvm::SmallVector<clang::FloatingLiteral*, 0> floatingLiterals_;
    llvm::SmallVector<llvm::APInt, 1> intValues_;
    llvm::SmallVector<llvm::APFloat, 0> floatValues_;
};

/**
 * Class to find out type information for a given qualified type.
 * Detects if a QualType has a typedef, is a complex type,
 * is a builtin type. For matches the type information is stored.
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

}  // end of namespace: mpi

#endif  // end of include guard: SUPPORTINGVISITORS_HPP_NWUC3OWQ
