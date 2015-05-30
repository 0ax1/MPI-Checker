/*
 The MIT License (MIT)

 Copyright (c) 2015 Alexander Droste

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
*/

#ifndef MPITYPES_HPP_IC7XR2MI
#define MPITYPES_HPP_IC7XR2MI

#include "llvm/ADT/SmallSet.h"
#include "StatementVisitor.hpp"
#include "CallExprVisitor.hpp"
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
    MPICall(const clang::CallExpr *const callExpr) : callExpr_{callExpr} {
        init(callExpr);
    };

    bool operator==(const MPICall &) const;
    bool operator!=(const MPICall &) const;

    // implicit conversion function
    operator const clang::IdentifierInfo *() const { return identInfo_; }

    const clang::CallExpr *callExpr() const { return callExpr_; }
    const std::vector<ArgumentVisitor> &arguments() const { return arguments_; }
    const clang::IdentifierInfo *identInfo() const { return identInfo_; }
    unsigned long id() const { return id_; };  // unique call identification

    // marking can be changed freely by clients
    // semantic depends on context of usage
    mutable bool isMarked_{false};
    mutable bool isReachable_{false};

private:
    /**
     * Init function shared by ctors.
     * @param callExpr mpi call captured
     */
    void init(const clang::CallExpr *const callExpr) {
        const clang::FunctionDecl *functionDeclNew =
            callExpr_->getDirectCallee();
        identInfo_ = functionDeclNew->getIdentifier();
        // build argument vector
        for (size_t i = 0; i < callExpr->getNumArgs(); ++i) {
            // emplace triggers ArgumentVisitor ctor
            arguments_.emplace_back(callExpr->getArg(i));
        }
    }

    const clang::CallExpr *callExpr_;
    std::vector<ArgumentVisitor> arguments_;
    const clang::IdentifierInfo *identInfo_;
    unsigned long id_{idCounter++};  // unique call identification

    static unsigned long idCounter;
};

// to capture rank variables
namespace MPIRank {
extern llvm::SmallSet<const clang::VarDecl *, 4> visitedRankVariables;
}

// to capture rank cases from branches
class MPIRankCase {
public:
    MPIRankCase(const clang::Stmt *const then,
                const clang::Stmt *const matchedCondition,
                const std::vector<ConditionVisitor> &unmatchedConditions,
                const MPIFunctionClassifier &funcClassifier)

        : unmatchedConditions_{unmatchedConditions} {
        if (matchedCondition) {
            matchedCondition_.reset(new ConditionVisitor{matchedCondition});
        }

        const CallExprVisitor callExprVisitor{then};  // collect call exprs
        for (const clang::CallExpr *const callExpr :
             callExprVisitor.callExprs()) {
            // add mpi calls only
            if (funcClassifier.isMPIType(
                    callExpr->getDirectCallee()->getIdentifier())) {
                mpiCalls_.push_back(callExpr);
            }
        }
    }

    static void unmarkCalls() {
        for (MPIRankCase &rankCase : MPIRankCase::visitedRankCases) {
            for (MPICall &call : rankCase.mpiCalls_) {
                call.isMarked_ = false;
            }
        }
    }

    bool isConditionAmbiguous() const;
    bool isConditionUnambiguouslyEqual(const MPIRankCase &) const;
    size_t size() const { return mpiCalls_.size(); }
    const std::vector<MPICall> &mpiCalls() const { return mpiCalls_; }
    const std::unique_ptr<ConditionVisitor> &matchedCondition() const {
        return matchedCondition_;
    }

    // conditions not fullfilled to enter rank case
    const std::vector<ConditionVisitor> unmatchedConditions_;
    static llvm::SmallVector<MPIRankCase, 8> visitedRankCases;

private:
    std::vector<MPICall> mpiCalls_;
    // condition fulfilled to enter rank case
    std::unique_ptr<ConditionVisitor> matchedCondition_{nullptr};
};

// for path sensitive analysis–––––––––––––––––––––––––––––––––––––––––––––––
struct RequestVar {
    RequestVar(const clang::VarDecl *const varDecl,
               const clang::CallExpr *const callExpr)
        : varDecl_{varDecl}, lastUser_{callExpr} {}

    void Profile(llvm::FoldingSetNodeID &id) const {
        id.AddPointer(varDecl_);
        id.AddPointer(lastUser_);
    }

    bool operator==(const RequestVar &toCompare) const {
        return toCompare.varDecl_ == varDecl_;
    }

    const clang::VarDecl *const varDecl_;
    const clang::CallExpr *const lastUser_;
};
}  // end of namespace: mpi

// register data structure for path sensitive analysis
REGISTER_MAP_WITH_PROGRAMSTATE(RequestVarMap, clang::VarDecl *, mpi::RequestVar)

#endif  // end of include guard: MPITYPES_HPP_IC7XR2MI
