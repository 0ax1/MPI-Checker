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

    MPICall(clang::CallExpr *callExpr,
            const clang::Stmt *const matchedCondition)
        : callExpr_{callExpr} {
        init(callExpr);
    };

    // implicit conversion function
    operator const clang::IdentifierInfo *() const { return identInfo_; }

    const clang::CallExpr *const callExpr_;
    const llvm::SmallVector<mpi::StmtVisitor, 8> arguments_;
    const clang::IdentifierInfo *identInfo_;
    const unsigned long id_{id++};  // unique call identification
    // marking can be changed freely by clients
    // semantic depends on context of usage
    mutable bool isMarked_;

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
            const_cast<llvm::SmallVector<mpi::StmtVisitor, 8> &>(arguments_)
                .emplace_back(callExpr->getArg(i));
        }
    }

    static unsigned long id;
};

// to capture rank variables
namespace MPIRank {
extern llvm::SmallSet<const clang::VarDecl *, 4> visitedRankVariables;
}

// to capture rank cases from branches
struct MPIRankCase {
    MPIRankCase(clang::Stmt *matchedCondition,
                llvm::SmallVector<clang::Stmt *, 4> unmatchedConditions) {

        // init stmt visitors with conditions
        if (matchedCondition) {
            matchedCondition_.reset(new StmtVisitor{matchedCondition});
            initConditionType();
        }
        for (auto const unmatchedCondition : unmatchedConditions) {
            unmatchedConditions.emplace_back(unmatchedCondition);
        }
    }

    bool isRankConditionEqual(MPIRankCase &);
    void initConditionType();
    bool isConditionTypeStandard();
    size_t size() { return mpiCalls_.size(); }

    std::vector<std::reference_wrapper<MPICall>> mpiCalls_;
    // condition fullfilled to enter rank case
    std::unique_ptr<StmtVisitor> matchedCondition_{nullptr};
    // conditions not fullfilled to enter rank case
    const llvm::SmallVector<StmtVisitor, 4> unmatchedConditions_;
    static llvm::SmallVector<MPIRankCase, 8> visitedRankCases;


private:
    bool isConditionTypeStandard_;
};

// for path sensitive analysis–––––––––––––––––––––––––––––––––––––––––––––––
struct RequestVar {
    clang::VarDecl *varDecl_;
    clang::CallExpr *lastUser_;

    RequestVar(clang::VarDecl *varDecl, clang::CallExpr *callExpr)
        : varDecl_{varDecl}, lastUser_{callExpr} {}

    void Profile(llvm::FoldingSetNodeID &id) const {
        id.AddPointer(varDecl_);
        id.AddPointer(lastUser_);
    }

    bool operator==(const RequestVar &toCompare) const {
        return toCompare.varDecl_ == varDecl_;
    }
};
}  // end of namespace: mpi

// register data structure for path sensitive analysis
REGISTER_MAP_WITH_PROGRAMSTATE(RequestVarMap, clang::VarDecl *, mpi::RequestVar)

#endif  // end of include guard: MPITYPES_HPP_IC7XR2MI
