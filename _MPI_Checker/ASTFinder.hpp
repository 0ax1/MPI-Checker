#ifndef ASTFINDER_HPP_LN8EJWWV
#define ASTFINDER_HPP_LN8EJWWV

#include "clang/AST/RecursiveASTVisitor.h"
#include "Typedefs.hpp"

/// A simple visitor to record what VarDecls occur in EH-handling code.
class FindVarDecl : public clang::RecursiveASTVisitor<FindVarDecl> {
public:
    std::vector<clang::Expr *> expressions_;
    bool floatIdentifierUsed{false};
    clang::FloatingLiteral *fl{nullptr};

    bool VisitDeclRefExpr(clang::DeclRefExpr *potentialVar) {
        if (clang::isa<clang::VarDecl>(potentialVar->getDecl())) {
            expressions_.push_back(potentialVar);
        }
        return true;
    }

    bool VisitBinaryOperator(clang::BinaryOperator *binOp) {
        expressions_.push_back(binOp);
        return true;
    }

    bool VisitIntegerLiteral(clang::IntegerLiteral *intLiteral) {
        expressions_.push_back(intLiteral);
        return true;
    }

    bool VisitFloatingLiteral(clang::FloatingLiteral *floatLiteral) {
        floatIdentifierUsed = true;
        fl = floatLiteral;
        return false;
    }
};

/// A simple visitor to record what VarDecls occur in EH-handling code.
// class SingleArgVisitor : public clang::RecursiveASTVisitor<SingleArgVisitor> {
// private:
    // clang::AnalysisDeclContext &analysisDeclContext_;

    // size_t index_{0};
    // size_t binOperatorStack_{0};

    // clang::Expr *arguments_[7];

    // // buffer is always given as variable
    // // clang::Expr *buffer_;
    // // clang::Expr *count_;
    // // clang::Expr *datatype_;
    // // clang::Expr *rank_;
    // // clang::Expr *tag_;
    // // clang::Expr *communicator_;
    // // clang::Expr *request_;

// public:
    // SingleArgVisitor(clang::AnalysisDeclContext &adc)
        // : analysisDeclContext_{adc} {}
    // std::vector<int> floatLiteralIndices_{};
    // std::vector<int> floatVarIndices_{};

    // void incrIndex() {
        // if (!binOperatorStack_) {
            // ++index_;
        // } else {
            // --binOperatorStack_;
        // }
    // }

    // bool VisitExpr(clang::Expr *expr) {
        // if (clang::IntegerLiteral *var =
                // clang::dyn_cast<clang::IntegerLiteral>(expr)) {
        // }
        // else if (clang::IntegerLiteral *var =
                // clang::dyn_cast<clang::IntegerLiteral>(expr)) {
        // }
        // else if (clang::IntegerLiteral *var =
                // clang::dyn_cast<clang::IntegerLiteral>(expr)) {
        // }
        // else if (clang::IntegerLiteral *var =
                // clang::dyn_cast<clang::IntegerLiteral>(expr)) {
        // }

        // // llvm::outs() << "log" << "\n";

        // // if (expr->getType().isConstant(analysisDeclContext_.getASTContext()))
        // // {
        // // expr->getType().dump();
        // // }

        // incrIndex();
        // return true;
    // }

    // bool VisitDeclRefExpr(clang::DeclRefExpr *potentialVar) {
        // if (clang::VarDecl *var =
                // clang::dyn_cast<clang::VarDecl>(potentialVar->getDecl())) {
            // if (!binOperatorStack_) {
            // }

            // if (var->getType()->isPointerType()) {
                // if (var->getType()->getPointeeType()->isFloatingType()) {
                    // floatVarIndices_.push_back(index_);
                // }
            // } else if (var->getType()->isFloatingType()) {
                // floatVarIndices_.push_back(index_);
            // }
            // incrIndex();
        // }
        // return true;
    // }

    // bool VisitBinaryOperator(clang::BinaryOperator *binOp) {
        // // binary operator has 2 operands -> -- for arg index
        // ++binOperatorStack_;

        // return true;
    // }

    // bool VisitIntegerLiteral(clang::IntegerLiteral *intLiteral) {
        // incrIndex();
        // return true;
    // }

    // bool VisitFloatingLiteral(clang::FloatingLiteral *floatLiteral) {
        // floatLiteralIndices_.push_back(index_);
        // // incrIndex();
        // return true;
    // }
// };

#endif  // end of include guard: ASTFINDER_HPP_LN8EJWWV
