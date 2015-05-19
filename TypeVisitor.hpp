#ifndef TYPEVISITOR_HPP_KOZYUVZH
#define TYPEVISITOR_HPP_KOZYUVZH

#include "MPIFunctionClassifier.hpp"

namespace mpi {

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

}  // end of namespace: mpi
#endif  // end of include guard: TYPEVISITOR_HPP_KOZYUVZH
