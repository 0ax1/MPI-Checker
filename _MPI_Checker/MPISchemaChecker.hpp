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

class MPI_ASTVisitor : public clang::RecursiveASTVisitor<MPI_ASTVisitor> {
    clang::ento::BugReporter &bugReporter_;
    const clang::ento::CheckerBase &checkerBase_;
    clang::AnalysisDeclContext &analysisDeclContext_;

public:
    MPI_ASTVisitor(clang::ento::BugReporter &bugReporter,
                   const clang::ento::CheckerBase &checkerBase,
                   clang::AnalysisDeclContext &analysisDeclContext)
        : bugReporter_{bugReporter},
          checkerBase_{checkerBase},
          analysisDeclContext_{analysisDeclContext} {
        identifierInit(analysisDeclContext.getASTContext());
    }

    // to enable classification of mpi-functions during analysis
    std::vector<clang::IdentifierInfo *> mpiSendTypes;
    std::vector<clang::IdentifierInfo *> mpiRecvTypes;

    std::vector<clang::IdentifierInfo *> mpiBlockingTypes;
    std::vector<clang::IdentifierInfo *> mpiNonBlockingTypes;

    std::vector<clang::IdentifierInfo *> mpiPointToPointTypes;
    std::vector<clang::IdentifierInfo *> mpiPointToCollTypes;
    std::vector<clang::IdentifierInfo *> mpiCollToPointTypes;
    std::vector<clang::IdentifierInfo *> mpiCollToCollTypes;
    std::vector<clang::IdentifierInfo *> mpiType;

    clang::IdentifierInfo *identInfo_MPI_Send{nullptr},
        *identInfo_MPI_Recv{nullptr}, *identInfo_MPI_Isend{nullptr},
        *identInfo_MPI_Irecv{nullptr}, *identInfo_MPI_Issend{nullptr},
        *identInfo_MPI_Ssend{nullptr}, *identInfo_MPI_Bsend{nullptr},
        *identInfo_MPI_Rsend{nullptr}, *identInfo_MPI_Comm_rank{nullptr},
        *IdentInfoTrackMem{nullptr};

    void identifierInit(clang::ASTContext &);

    // visitor callbacks
    bool VisitDecl(clang::Decl *);
    bool VisitFunctionDecl(clang::FunctionDecl *);
    bool VisitDeclRefExpr(clang::DeclRefExpr *);
    bool VisitCallExpr(clang::CallExpr *);
    // TODO
    // bool VisitIfStmt(const IfStmt *);

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

    const clang::Type *getBuiltinType(const clang::ValueDecl *) const;

    void checkForDuplicate(MPICall &) const;
    void checkForFloatArgs(MPICall &);

    void reportFloat(clang::CallExpr *, size_t, FloatArgType) const;
    void reportDuplicate(clang::CallExpr *, clang::CallExpr *) const;

    bool fullArgumentComparison(MPICall &, MPICall &, size_t) const;
};

}  // end of namespace: mpi

#endif  // end of include guard: MPISCHEMACHECKERAST_HPP_NKN9I06D
