#ifndef RANKVISITOR_HPP_WZL2H4SR
#define RANKVISITOR_HPP_WZL2H4SR

#include "MPIFunctionClassifier.hpp"

namespace mpi {

/**
 * Visitor class to collect rank variables.
 */
class RankVisitor : public clang::RecursiveASTVisitor<RankVisitor> {
public:
    RankVisitor(clang::ento::AnalysisManager &analysisManager)
        : funcClassifier_{analysisManager} {}

    // collect rank vars
    bool VisitCallExpr(clang::CallExpr *callExpr) {
        if (funcClassifier_.isMPIType(
                callExpr->getDirectCallee()->getIdentifier())) {
            MPICall mpiCall{callExpr};
            if (funcClassifier_.isMPI_Comm_rank(mpiCall)) {
                clang::VarDecl *varDecl = mpiCall.arguments()[1].vars()[0];
                MPIRank::visitedRankVariables.insert(varDecl);
            }
        }

        return true;
    }

private:
    MPIFunctionClassifier funcClassifier_;
};

}  // end of namespace: mpi
#endif  // end of include guard: RANKVISITOR_HPP_WZL2H4SR
