#ifndef CALLEXPRVISITOR_HPP_7XGM9NP3
#define CALLEXPRVISITOR_HPP_7XGM9NP3

namespace mpi {

class CallExprVisitor : public clang::RecursiveASTVisitor<CallExprVisitor> {
public:
    CallExprVisitor(const clang::Stmt *const stmt) {
        TraverseStmt(const_cast<clang::Stmt*>(stmt));
    }

    bool VisitCallExpr(clang::CallExpr *callExpr) {
        callExprs_.push_back(callExpr);
        return true;
    }

    const llvm::SmallVectorImpl<clang::CallExpr *> &callExprs() const {
        return callExprs_;
    }

private:
    llvm::SmallVector<clang::CallExpr *, 8> callExprs_;
};

}  // end of namespace: mpi

#endif  // end of include guard: CALLEXPRVISITOR_HPP_7XGM9NP3
