#ifndef MPISCHEMACHECKERAST_HPP_NKN9I06D
#define MPISCHEMACHECKERAST_HPP_NKN9I06D

#include "../ClangSACheckers.h"
#include "clang/StaticAnalyzer/Core/Checker.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"
#include "clang/AST/RecursiveASTVisitor.h"

#include <vector>
#include <string>

#include "SupportingVisitors.hpp"
#include "MPIFunctionClassifier.cpp"
#include "MPIBugReporter.hpp"
#include "Typedefs.hpp"

namespace mpi {

// forward declaration
struct MPICall;

// callback functions a checker can register for
// http://clang.llvm.org/doxygen/CheckerDocumentation_8cpp_source.html

// dump-color legend
// Red           - CastColor
// Green         - TypeColor
// Bold Green    - DeclKindNameColor, UndeserializedColor
// Yellow        - AddressColor, LocationColor
// Blue          - CommentColor, NullColor, IndentColor
// Bold Blue     - AttrColor
// Bold Magenta  - StmtColor
// Cyan          - ValueKindColor, ObjectKindColor
// Bold Cyan     - ValueColor, DeclNameColor


class MPIVisitor : public clang::RecursiveASTVisitor<MPIVisitor> {
private:
    // validation functions
    bool fullArgumentComparison(const MPICall &, const MPICall &, size_t) const;

    void checkForInvalidArgs(const MPICall &) const;
    void checkForDuplicatePointToPoint(const MPICall &) const;

    void checkBufferTypeMatch(const MPICall &mpiCall) const;
    void matchBoolType(clang::CallExpr *, vis::TypeVisitor &,
                       llvm::StringRef) const;
    void matchCharType(clang::CallExpr *, vis::TypeVisitor &,
                       llvm::StringRef) const;
    void matchSignedType(clang::CallExpr *, vis::TypeVisitor &,
                         llvm::StringRef) const;
    void matchUnsignedType(clang::CallExpr *, vis::TypeVisitor &,
                           llvm::StringRef) const;
    void matchFloatType(clang::CallExpr *, vis::TypeVisitor &,
                        llvm::StringRef) const;
    void matchComplexType(clang::CallExpr *, vis::TypeVisitor &,
                          llvm::StringRef) const;
    void matchExactWidthType(clang::CallExpr *, vis::TypeVisitor &,
                             llvm::StringRef) const;

    MPIFunctionClassifier funcClassifier_;
    MPIBugReporter bugReporter_;
    clang::ento::AnalysisManager &analysisManager_;

public:
    enum class MatchType { kMatch, kMismatch, kNoMatch };

    MPIVisitor(clang::ento::BugReporter &bugReporter,
               const clang::ento::CheckerBase &checkerBase,
               clang::ento::AnalysisManager &analysisManager)
        : funcClassifier_{analysisManager},
          bugReporter_{bugReporter, checkerBase, analysisManager},
          analysisManager_{analysisManager} {}

    // visitor callbacks
    bool VisitDecl(clang::Decl *);
    bool VisitFunctionDecl(clang::FunctionDecl *);
    bool VisitDeclRefExpr(clang::DeclRefExpr *);
    bool VisitCallExpr(clang::CallExpr *);
    bool VisitIfStmt(clang::IfStmt *);

    void checkForDuplicates() const;
};

}  // end of namespace: mpi

#endif  // end of include guard: MPISCHEMACHECKERAST_HPP_NKN9I06D
