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

#ifndef MPIVARIABLEVISITOR_HPP_WZL2H4SR
#define MPIVARIABLEVISITOR_HPP_WZL2H4SR

#include "MPIFunctionClassifier.hpp"
#include "Utility.hpp"

namespace mpi {

/**
 * Visitor class to collect rank variables.
 */
class MPIVariableVisitor
    : public clang::RecursiveASTVisitor<MPIVariableVisitor> {
public:
    MPIVariableVisitor(clang::ento::AnalysisManager &analysisManager)
        : funcClassifier_{analysisManager} {}

    // TODO what if variable is in array?

    // collect rank vars
    bool VisitCallExpr(clang::CallExpr *callExpr) {
        if (funcClassifier_.isMPIType(util::getIdentInfo(callExpr))) {
            MPICall mpiCall{callExpr};

            // rank variable
            if (funcClassifier_.isMPI_Comm_rank(mpiCall)) {

                if (mpiCall.arguments_[1].members_.size()) {
                    MPIRank::variables.insert(
                        mpiCall.arguments_[1].members_[0]);
                } else {
                    MPIRank::variables.insert(
                        mpiCall.arguments_[1].vars_[0]);
                }
            }
            // process count variable
            else if (funcClassifier_.isMPI_Comm_size(mpiCall)) {
                if (mpiCall.arguments_[1].members_.size()) {
                    MPIProcessCount::variables.insert(
                        mpiCall.arguments_[1].members_[0]);
                } else {
                    MPIProcessCount::variables.insert(
                        mpiCall.arguments_[1].vars_[0]);
                }
            }
        }

        return true;
    }

private:
    MPIFunctionClassifier funcClassifier_;
};

}  // end of namespace: mpi
#endif  // end of include guard: MPIVARIABLEVISITOR_HPP_WZL2H4SR
