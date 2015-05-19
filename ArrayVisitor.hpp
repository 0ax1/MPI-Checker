#ifndef ARRAYVISITOR_HPP_LNSPXQ6N
#define ARRAYVISITOR_HPP_LNSPXQ6N

#include "clang/AST/RecursiveASTVisitor.h"

namespace mpi {

class ArrayVisitor : public clang::RecursiveASTVisitor<ArrayVisitor> {
public:
    ArrayVisitor(clang::VarDecl *varDecl) : arrayVarDecl_{varDecl} {
        TraverseVarDecl(arrayVarDecl_);
    }
    // must be public to trigger callbacks
    bool VisitDeclRefExpr(clang::DeclRefExpr *declRef) {
        if (clang::VarDecl *var =
                clang::dyn_cast<clang::VarDecl>(declRef->getDecl())) {
            vars_.push_back(var);
        }
        return true;
    }

    const clang::VarDecl *arrayVarDecl() { return arrayVarDecl_; }
    const llvm::SmallVectorImpl<clang::VarDecl *> &vars() { return vars_; }

private:
    // complete VarDecl
    clang::VarDecl *arrayVarDecl_;
    // array variables
    llvm::SmallVector<clang::VarDecl *, 4> vars_;
};

}  // end of namespace: mpi
#endif  // end of include guard: ARRAYVISITOR_HPP_LNSPXQ6N
