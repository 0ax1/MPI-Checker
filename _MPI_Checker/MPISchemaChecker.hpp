#ifndef MPISCHEMACHECKERAST_HPP_NKN9I06D
#define MPISCHEMACHECKERAST_HPP_NKN9I06D

#include "../ClangSACheckers.h"
#include "clang/StaticAnalyzer/Core/Checker.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"
#include "clang/AST/RecursiveASTVisitor.h"

#include <vector>
#include <string>

#include "SupportingVisitors.hpp"
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

class MPIFunctionClassifier {
public:
    MPIFunctionClassifier(clang::ento::AnalysisManager &analysisManager) {
        identifierInit(analysisManager);
    }
    // to enable classification of mpi-functions during analysis
    // to inspect properties of mpi functions
    bool isMPIType(const clang::IdentifierInfo *) const;
    bool isSendType(const clang::IdentifierInfo *) const;
    bool isRecvType(const clang::IdentifierInfo *) const;
    bool isBlockingType(const clang::IdentifierInfo *) const;
    bool isNonBlockingType(const clang::IdentifierInfo *) const;
    bool isPointToPointType(const clang::IdentifierInfo *) const;
    bool isPointToCollType(const clang::IdentifierInfo *) const;
    bool isCollToPointType(const clang::IdentifierInfo *) const;
    bool isCollToCollType(const clang::IdentifierInfo *) const;

private:
    void identifierInit(clang::ento::AnalysisManager &);

    // to enable classification of mpi-functions during analysis
    std::vector<clang::IdentifierInfo *> mpiSendTypes_;
    std::vector<clang::IdentifierInfo *> mpiRecvTypes_;

    std::vector<clang::IdentifierInfo *> mpiBlockingTypes_;
    std::vector<clang::IdentifierInfo *> mpiNonBlockingTypes_;

    std::vector<clang::IdentifierInfo *> mpiPointToPointTypes_;
    std::vector<clang::IdentifierInfo *> mpiPointToCollTypes_;
    std::vector<clang::IdentifierInfo *> mpiCollToPointTypes_;
    std::vector<clang::IdentifierInfo *> mpiCollToCollTypes_;
    std::vector<clang::IdentifierInfo *> mpiType_;

    clang::IdentifierInfo *identInfo_MPI_Send_{nullptr},
        *identInfo_MPI_Recv_{nullptr}, *identInfo_MPI_Isend_{nullptr},
        *identInfo_MPI_Irecv_{nullptr}, *identInfo_MPI_Issend_{nullptr},
        *identInfo_MPI_Ssend_{nullptr}, *identInfo_MPI_Bsend_{nullptr},
        *identInfo_MPI_Rsend_{nullptr}, *identInfo_MPI_Comm_rank_{nullptr};
};

class MPIBugReporter {
private:
    clang::ento::BugReporter &bugReporter_;
    const clang::ento::CheckerBase &checkerBase_;
    clang::ento::AnalysisManager &analysisManager_;

public:
    MPIBugReporter(clang::ento::BugReporter &bugReporter,
                   const clang::ento::CheckerBase &checkerBase,
                   clang::ento::AnalysisManager &analysisManager)
        : bugReporter_{bugReporter},
          checkerBase_{checkerBase},
          analysisManager_{analysisManager} {}

    void reportTypeMismatch(clang::CallExpr *) const;
    void reportInvalidArgumentType(clang::CallExpr *, size_t,
                                   InvalidArgType) const;
    void reportDuplicate(const clang::CallExpr *,
                         const clang::CallExpr *) const;

    clang::Decl *currentFunctionDecl_{nullptr};
};

class MPI_ASTVisitor : public clang::RecursiveASTVisitor<MPI_ASTVisitor> {
private:
    // validation functions
    bool fullArgumentComparison(const MPICall &, const MPICall &, size_t) const;

    void checkForInvalidArgs(const MPICall &) const;
    void checkForDuplicatePointToPoint(const MPICall &) const;

    void checkBufferTypeMatch(const MPICall &mpiCall) const;
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

    MPI_ASTVisitor(clang::ento::BugReporter &bugReporter,
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
