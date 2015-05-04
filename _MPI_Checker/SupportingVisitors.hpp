#ifndef SUPPORTINGVISITORS_HPP_NWUC3OWQ
#define SUPPORTINGVISITORS_HPP_NWUC3OWQ

#include "../ClangSACheckers.h"
#include "clang/StaticAnalyzer/Core/Checker.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"
#include "clang/AST/RecursiveASTVisitor.h"

#include "Typedefs.hpp"

namespace vis {

class SingleArgVisitor : public clang::RecursiveASTVisitor<SingleArgVisitor> {
public:
    SingleArgVisitor(clang::CallExpr *argExpression, size_t idx)
        : expr_{argExpression->getArg(idx)} {
        TraverseStmt(expr_);
    }

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
    llvm::SmallVector<llvm::APInt, 1> integerLiterals_;
    llvm::SmallVector<llvm::APFloat, 0> floatingLiterals_;
    // if all operands are static
    bool isArgumentStatic_{true};
    // no operator, single literal or variable
    bool isSimpleExpression_{true};
};

class TypeVisitor : public clang::RecursiveASTVisitor<TypeVisitor> {
public:
    TypeVisitor(clang::QualType qualType) {
        TraverseType(qualType);
    }

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

    // clang::BuiltinType *builtinType_{nullptr};
};

}  // end of namespace: vis

#endif  // end of include guard: SUPPORTINGVISITORS_HPP_NWUC3OWQ
