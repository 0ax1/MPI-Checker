#include "SupportingVisitors.hpp"

using namespace clang;
using namespace ento;

namespace vis {

// variables or functions
bool SingleArgVisitor::VisitDeclRefExpr(clang::DeclRefExpr *declRef) {
    if (clang::VarDecl *var =
            clang::dyn_cast<clang::VarDecl>(declRef->getDecl())) {
        vars_.push_back(var);
        isArgumentStatic_ = false;
    } else if (clang::FunctionDecl *fn =
                   clang::dyn_cast<clang::FunctionDecl>(declRef->getDecl())) {
        functions_.push_back(fn);
        isArgumentStatic_ = false;
    }
    return true;
}

bool SingleArgVisitor::VisitBinaryOperator(clang::BinaryOperator *op) {
    binaryOperators_.push_back(op->getOpcode());
    isSimpleExpression_ = false;
    return true;
}

bool SingleArgVisitor::VisitIntegerLiteral(IntegerLiteral *intLiteral) {
    integerLiterals_.push_back(intLiteral->getValue());
    return true;
}

bool SingleArgVisitor::VisitFloatingLiteral(FloatingLiteral *floatLiteral) {
    floatingLiterals_.push_back(floatLiteral->getValue());
    return true;
}

}  // end of namespace: vis
