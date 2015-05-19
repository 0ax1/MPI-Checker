#include "Visitors.hpp"
#include "Container.hpp"
#include "MPITypes.hpp"

using namespace clang;
using namespace ento;

namespace mpi {

bool ArrayVisitor::VisitDeclRefExpr(clang::DeclRefExpr *declRef) {
    if (clang::VarDecl *var =
            clang::dyn_cast<clang::VarDecl>(declRef->getDecl())) {
        vars_.push_back(var);
    }
    return true;
}

bool RankVisitor::VisitCallExpr(CallExpr *callExpr) {
    if (funcClassifier_.isMPIType(
            callExpr->getDirectCallee()->getIdentifier())) {
        MPICall mpiCall{callExpr};
        if (funcClassifier_.isMPI_Comm_rank(mpiCall)) {
            VarDecl *varDecl = mpiCall.arguments()[1].vars()[0];
            MPIRank::visitedRankVariables.insert(varDecl);
        }
    }

    return true;
}

}  // end of namespace: mpi
