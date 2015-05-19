#ifndef SUPPORTINGVISITORS_HPP_NWUC3OWQ
#define SUPPORTINGVISITORS_HPP_NWUC3OWQ

#include "clang/AST/RecursiveASTVisitor.h"
#include "MPIFunctionClassifier.hpp"
#include "StmtVisitor.hpp"

namespace mpi {

class ArrayVisitor : public clang::RecursiveASTVisitor<ArrayVisitor> {
public:
    ArrayVisitor(clang::VarDecl *varDecl) : arrayVarDecl_{varDecl} {
        TraverseVarDecl(arrayVarDecl_);
    }
    // must be public to trigger callbacks
    bool VisitDeclRefExpr(clang::DeclRefExpr *);

    const clang::VarDecl *arrayVarDecl() { return arrayVarDecl_; }
    const llvm::SmallVector<clang::VarDecl *, 4> &vars() { return vars_; }

private:
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
    TypeVisitor(clang::QualType qualType) : qualType_{qualType} {
        TraverseType(qualType);
    }

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
    const clang::QualType qualType_;
    bool isTypedefType() const { return isTypedefType_; }
    const std::string typedefTypeName() const & { return typedefTypeName_; }
    const clang::BuiltinType *builtinType() const { return builtinType_; }
    const clang::ComplexType *complexType() const { return complexType_; }

private:
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
