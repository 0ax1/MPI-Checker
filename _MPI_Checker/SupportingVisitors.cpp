#include "SupportingVisitors.hpp"

using namespace clang;
using namespace ento;

namespace mpi {

// variables or functions can be a declrefexpr
bool SingleArgVisitor::VisitDeclRefExpr(clang::DeclRefExpr *declRef) {
    if (clang::VarDecl *var =
            clang::dyn_cast<clang::VarDecl>(declRef->getDecl())) {
        vars_.push_back(var);
    } else if (clang::FunctionDecl *fn =
                   clang::dyn_cast<clang::FunctionDecl>(declRef->getDecl())) {
        functions_.push_back(fn);
    }
    return true;
}

bool SingleArgVisitor::VisitBinaryOperator(clang::BinaryOperator *op) {
    binaryOperators_.push_back(op->getOpcode());
    return true;
}

bool SingleArgVisitor::VisitIntegerLiteral(IntegerLiteral *intLiteral) {
    integerLiterals_.push_back(intLiteral);
    intValues_.push_back(intLiteral->getValue());
    return true;
}

bool SingleArgVisitor::VisitFloatingLiteral(FloatingLiteral *floatLiteral) {
    floatingLiterals_.push_back(floatLiteral);
    floatValues_.push_back(floatLiteral->getValue());
    return true;
}

}  // end of namespace: mpi
