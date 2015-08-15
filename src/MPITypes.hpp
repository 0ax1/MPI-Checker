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

#include "clang/StaticAnalyzer/Core/PathSensitive/CallEvent.h"
#include "llvm/ADT/SmallSet.h"
#include "StatementVisitor.hpp"
#include "CallExprVisitor.hpp"
#include "MPIFunctionClassifier.hpp"
#include "Utility.hpp"
#include "Container.hpp"

// types modeling mpi function calls and variables –––––––––––––––––––––

namespace mpi {
// argument schema enums –––––––––––––––––––––––––––––––––––––––––––––––
// scope enums, but keep weak typing to make values usable as indices
namespace MPIPointToPoint {
// valid for all point to point functions
enum { kBuf, kCount, kDatatype, kRank, kTag, kComm, kRequest };
}

namespace MPIRank {
// to capture rank variables
extern llvm::SmallSet<const clang::ValueDecl *, 4> variables;
extern const std::string encoding;
}

namespace MPIProcessCount {
// to capture process count variables
extern llvm::SmallSet<const clang::ValueDecl *, 4> variables;
extern const std::string encoding;
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
    const clang::IdentifierInfo *identInfo() const { return identInfo_; }
    unsigned long id() const { return id_; };  // unique call identification
    const llvm::SmallVector<ArgumentVisitor, 6> &arguments() const {
        return arguments_;
    };

    // marking can be changed freely by clients
    // semantics depend on context of usage
    mutable bool isMarked_{false};
    mutable bool isReachable_{false};

private:
    void init(const clang::CallExpr *const);

    const clang::CallExpr *callExpr_;
    llvm::SmallVector<ArgumentVisitor, 6> arguments_;
    const clang::IdentifierInfo *identInfo_;
    unsigned long id_{idCounter++};  // unique call identification

    static unsigned long idCounter;
};

// to capture rank cases from branches
class MPIRankCase {
public:
    MPIRankCase(
        const clang::Stmt *const then,
        const clang::Stmt *const matchedCondition,
        const llvm::SmallVector<ConditionVisitor, 2> &unmatchedConditions,
        const MPIFunctionClassifier &funcClassifier)

        : unmatchedConditions_{unmatchedConditions} {
        setupConditions(matchedCondition);
        setupMPICallsFromBody(then, funcClassifier);
        identifySpecialRanks();
    }

    static void unmarkCalls();

    bool isRankAmbiguous() const;
    bool isRankUnambiguouslyEqual(const MPIRankCase &) const;

    bool isFirstRank() const { return isFirstRank_; }
    bool isLastRank() const { return isLastRank_; }
    const llvm::SmallVector<MPICall, 16> &mpiCalls() const { return mpiCalls_; }
    const llvm::SmallVector<ConditionVisitor, 8> &conditions() const {
        return conditions_;
    }
    const llvm::SmallVector<ConditionVisitor, 4> &rankConditions() const {
        return rankConditions_;
    }

    // conditions not fullfilled to enter rank case
    const llvm::SmallVector<ConditionVisitor, 2> unmatchedConditions_;
    static llvm::SmallVector<MPIRankCase, 8> cases;

private:
    void setupConditions(const clang::Stmt *const);
    void setupMPICallsFromBody(const clang::Stmt *const,
                               const MPIFunctionClassifier &);
    void identifySpecialRanks();

    bool isFirstRank_{false};
    bool isLastRank_{false};

    llvm::SmallVector<MPICall, 16> mpiCalls_;
    // dissected conditions
    llvm::SmallVector<ConditionVisitor, 8> conditions_;
    // subset containing conditions with rank vars
    llvm::SmallVector<ConditionVisitor, 4> rankConditions_;
    // condition fulfilled to enter rank case
    std::unique_ptr<ConditionVisitor> completeCondition_{nullptr};
};

// for path sensitive analysis–––––––––––––––––––––––––––––––––––––––––––––––
struct RequestVar {
    RequestVar(const clang::ento::MemRegion *const memRegion,
               const clang::ento::CallEventRef<> callEvent)
        : memRegion_{memRegion}, lastUser_{callEvent} { }

    void Profile(llvm::FoldingSetNodeID &id) const {
        id.AddPointer(memRegion_);
        id.AddPointer(lastUser_->getOriginExpr());
    }

    bool operator==(const RequestVar &toCompare) const {
        return toCompare.memRegion_ == memRegion_;
    }

    const clang::ento::MemRegion *const memRegion_;
    const clang::ento::CallEventRef<> lastUser_;
};
}  // end of namespace: mpi
// TODO track request arrays (check bind?)

// register data structure for path sensitive analysis
REGISTER_MAP_WITH_PROGRAMSTATE(RequestVarMap, const clang::ento::MemRegion *,
                               mpi::RequestVar)

#endif  // end of include guard: MPITYPES_HPP_IC7XR2MI
