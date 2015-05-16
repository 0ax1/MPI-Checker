#include "SupportingVisitors.hpp"
#include <string>

using namespace clang;
using namespace ento;

namespace mpi {

// variables or functions can be a declrefexpr
bool StmtVisitor::VisitDeclRefExpr(clang::DeclRefExpr *declRef) {
    if (clang::VarDecl *var =
            clang::dyn_cast<clang::VarDecl>(declRef->getDecl())) {
        vars_.push_back(var);
        declarations_.push_back(var);
        sequentialSeries_.push_back(declRef);
    } else if (clang::FunctionDecl *fn =
                   clang::dyn_cast<clang::FunctionDecl>(declRef->getDecl())) {
        functions_.push_back(fn);
        declarations_.push_back(fn);
        sequentialSeries_.push_back(declRef);
    }
    return true;
}

bool StmtVisitor::VisitBinaryOperator(clang::BinaryOperator *op) {
    binaryOperators_.push_back(op->getOpcode());
    sequentialSeries_.push_back(op);
    return true;
}

bool StmtVisitor::VisitIntegerLiteral(IntegerLiteral *intLiteral) {
    integerLiterals_.push_back(intLiteral);
    intValues_.push_back(intLiteral->getValue());
    return true;
}

bool StmtVisitor::VisitFloatingLiteral(FloatingLiteral *floatLiteral) {
    floatingLiterals_.push_back(floatLiteral);
    floatValues_.push_back(floatLiteral->getValue());
    return true;
}

bool StmtVisitor::VisitCallExpr(clang::CallExpr *callExpr) {
    callExprs_.push_back(callExpr);
    return true;
};

bool ArrayVisitor::VisitDeclRefExpr(clang::DeclRefExpr *declRef) {
    if (clang::VarDecl *var =
            clang::dyn_cast<clang::VarDecl>(declRef->getDecl())) {
        vars_.push_back(var);
    }
    return true;
}

}  // end of namespace: mpi
