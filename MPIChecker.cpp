#include "TranslationUnitVisitor.hpp"
#include "MPICheckerPathSensitive.hpp"
#include "RankVisitor.hpp"

using namespace clang;
using namespace ento;

namespace mpi {

/**
 * Checker host class that registers the checker for static analysis.
 * Class name determines checker name to specify on the command line.
 * Is created once for every translation unit.
 */
class MPIChecker
    : public Checker<check::ASTDecl<TranslationUnitDecl>,
                     check::PreStmt<CallExpr>, check::EndFunction> {
public:
    // ast callback–––––––––––––––––––––––––––––––––––––––––––––––––––––––
    void checkASTDecl(const TranslationUnitDecl *tuDecl,
                      AnalysisManager &analysisManager,
                      BugReporter &bugReporter) const {
        // identify rank variables first
        RankVisitor rankVisitor{analysisManager};
        rankVisitor.TraverseTranslationUnitDecl(
            const_cast<TranslationUnitDecl *>(tuDecl));

        // traverse translation unit ast
        TranslationUnitVisitor visitor{bugReporter, *this, analysisManager};
        visitor.TraverseTranslationUnitDecl(
            const_cast<TranslationUnitDecl *>(tuDecl));

        // check after tu traversal
        visitor.checkerAST_.checkPointToPointSchema();
        visitor.checkerAST_.checkReachbility();
        visitor.checkerAST_.checkForRedundantCalls();

        // clear after every translation unit
        MPIRank::visitedRankVariables.clear();
        MPIRankCase::visitedRankCases.clear();
    }

    // path sensitive callbacks––––––––––––––––––––––––––––––––––––––––––––
    void checkPreStmt(const CallExpr *callExpr, CheckerContext &ctx) const {
        dynamicInit(ctx);
        checkerSens_->checkWaitUsage(callExpr, ctx);
        checkerSens_->checkDoubleNonblocking(callExpr, ctx);
    }

    void checkEndFunction(CheckerContext &ctx) const {
        // true if the current LocationContext has no caller context
        if (ctx.inTopFrame()) {
            dynamicInit(ctx);
            checkerSens_->checkMissingWaits(ctx);
            checkerSens_->clearRequestVars(ctx);
        }
    }

private:
    const std::unique_ptr<MPICheckerPathSensitive> checkerSens_;

    void dynamicInit(CheckerContext &ctx) const {
        if (!checkerSens_) {
            const_cast<std::unique_ptr<MPICheckerPathSensitive> &>(checkerSens_)
                .reset(new MPICheckerPathSensitive(ctx.getAnalysisManager(),
                                                   this, ctx.getBugReporter()));
        }
    }
};

}  // end of namespace: mpi

void ento::registerMPIChecker(CheckerManager &mgr) {
    mgr.registerChecker<mpi::MPIChecker>();
}
