#ifndef MPITYPES_HPP_IC7XR2MI
#define MPITYPES_HPP_IC7XR2MI

#include "SupportingVisitors.hpp"
#include "MPIFunctionClassifier.hpp"

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

    bool operator==(const MPICall &);
    bool operator!=(const MPICall &);
    // implicit conversion function
    operator const clang::IdentifierInfo *() const { return identInfo_; }

    const clang::CallExpr *callExpr_;
    std::vector<StmtVisitor> arguments_;
    const clang::IdentifierInfo *identInfo_;
    unsigned long id_{id++};  // unique call identification
    // marking can be changed freely by clients
    // semantic depends on context of usage
    mutable bool isMarked_{false};
    mutable bool isReachable_{false};

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
            arguments_.emplace_back(callExpr->getArg(i));
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
    MPIRankCase(clang::Stmt *then, clang::Stmt *matchedCondition,
                const std::vector<StmtVisitor> &unmatchedConditions,
                MPIFunctionClassifier &funcClassifier)

        : unmatchedConditions_{unmatchedConditions} {
        if (matchedCondition) {
            matchedCondition_.reset(new StmtVisitor{matchedCondition});
        }

        StmtVisitor stmtVisitor{then};  // collect call exprs
        for (clang::CallExpr *callExpr : stmtVisitor.callExprs_) {
            // add mpi calls only
            if (funcClassifier.isMPIType(
                    callExpr->getDirectCallee()->getIdentifier())) {
                mpiCalls_.push_back(callExpr);
            }
        }
    }

    bool isConditionAmbiguous();
    bool isConditionUnambiguouslyEqual(MPIRankCase &);
    size_t size() { return mpiCalls_.size(); }
    static void unmarkCalls() {
        for (MPIRankCase &rankCase : MPIRankCase::visitedRankCases) {
            for (MPICall &call : rankCase.mpiCalls_) {
                call.isMarked_ = false;
            }
        }
    }

    std::vector<MPICall> mpiCalls_;
    // condition fullfilled to enter rank case
    std::unique_ptr<StmtVisitor> matchedCondition_{nullptr};
    // conditions not fullfilled to enter rank case
    std::vector<StmtVisitor> unmatchedConditions_;
    static llvm::SmallVector<MPIRankCase, 8> visitedRankCases;
};

// for path sensitive
// analysis–––––––––––––––––––––––––––––––––––––––––––––––
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
