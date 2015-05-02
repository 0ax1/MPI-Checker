#include "SupportingVisitors.hpp"

using namespace clang;
using namespace ento;

namespace mpi {

bool SingleArgVisitor::VisitVarDecl(clang::VarDecl *var) {
    vars_.push_back(var);
    isArgumentStatic_ = false;
    return true;
}

bool SingleArgVisitor::VisitFunctionDecl(clang::FunctionDecl *fnDecl) {
    functions_.push_back(fnDecl);
    isArgumentStatic_ = false;
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

}  // end of namespace: mpi
