#ifndef MPITYPES_HPP_IC7XR2MI
#define MPITYPES_HPP_IC7XR2MI

#include "SupportingVisitors.hpp"

// types modeling mpi function calls and variables –––––––––––––––––––––

namespace mpi {
// argument schema enums –––––––––––––––––––––––––––––––––––––––––––––––
// scope enums, but keep weak typing to make values usable as indices
namespace MPIPointToPoint {
// valid for all point to point functions
enum { kBuf, kCount, kDatatype, kRank, kTag, kComm, kRequest };
}

struct MPICall {
public:
    MPICall(clang::CallExpr *callExpr) : callExpr_{callExpr} {
        init(callExpr);
    };

    MPICall(clang::CallExpr *callExpr, const clang::Stmt *const rankCondition)
        : callExpr_{callExpr}, rankCondition_{rankCondition} {
        init(callExpr);
    };

    MPICall(clang::CallExpr *callExpr, const clang::Stmt *const rankCondition,
            llvm::SmallVector<clang::Stmt *, 4> unmatchedRankConditions)
        : callExpr_{callExpr},
          rankCondition_{rankCondition},
          unmatchedRankConditions_{unmatchedRankConditions} {
        init(callExpr);
    };

    // implicit conversion function
    operator const clang::IdentifierInfo *() const { return identInfo_; }

    const clang::CallExpr *const callExpr_;
    const llvm::SmallVector<mpi::ExprVisitor, 8> arguments_;
    const clang::IdentifierInfo *identInfo_;
    const unsigned long id_{id++};  // unique call identification
    // marking can be changed freely by clients
    // semantic depends on context of usage
    mutable bool isMarked_;

    // if mpi call is executed in (if)/(else if) case
    const clang::Stmt *const rankCondition_{nullptr};
    // if mpi call is executed in (else) case
    const llvm::SmallVector<clang::Stmt *, 4> unmatchedRankConditions_;

    // to capture all visited calls traversing the ast
    static llvm::SmallVector<MPICall, 16> visitedCalls;

private:
    /**
     * Init function shared by ctors.
     * @param callExpr mpi call captured
     */
    void init(clang::CallExpr *callExpr) {
        const clang::FunctionDecl *functionDeclNew =
            callExpr_->getDirectCallee();
        identInfo_ = functionDeclNew->getIdentifier();
        // build argument vector
        for (size_t i = 0; i < callExpr->getNumArgs(); ++i) {
            // emplace triggers ExprVisitor ctor
            const_cast<llvm::SmallVector<mpi::ExprVisitor, 8> &>(arguments_)
                .emplace_back(callExpr->getArg(i));
        }
    }

    static unsigned long id;
};

// to capture request variables
struct MPIRequest {
    const clang::VarDecl *requestVariable_;
    const clang::CallExpr *callUsingTheRequest_;
    static llvm::SmallVector<MPIRequest, 4> visitedRequests;
};

// to capture rank variables
namespace MPIRank {
extern llvm::SmallSet<const clang::VarDecl *, 4> visitedRankVariables;
}

// to capture rank cases from branches
typedef std::vector<std::reference_wrapper<MPICall>> MPIrankCase;
namespace MPIRankCases {
extern llvm::SmallVector<MPIrankCase, 8> visitedRankCases;
}

// for path sensitive analysis–––––––––––––––––––––––––––––––––––––––––––––––
struct RankVar {
    clang::VarDecl *varDecl_;
    clang::CallExpr *lastUser_;

    RankVar(clang::VarDecl *varDecl, clang::CallExpr *callExpr) :
        varDecl_{varDecl}, lastUser_{callExpr} {}

    void Profile(llvm::FoldingSetNodeID &id) const {
        id.AddPointer(varDecl_);
        id.AddPointer(lastUser_);
    }

    bool operator==(const RankVar& toCompare) const {
        return toCompare.varDecl_ == varDecl_;
    }
};
}  // end of namespace: mpi

// register data structure for path sensitive analysis
REGISTER_MAP_WITH_PROGRAMSTATE(RankVarMap, clang::VarDecl *, mpi::RankVar)

#endif  // end of include guard: MPITYPES_HPP_IC7XR2MI
