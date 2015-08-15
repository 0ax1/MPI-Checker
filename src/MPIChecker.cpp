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

#include "clang/StaticAnalyzer/Core/PathSensitive/CallEvent.h"
#include "TranslationUnitVisitor.hpp"
#include "MPICheckerPathSensitive.hpp"
#include "MPIVariableVisitor.hpp"

using namespace clang;
using namespace ento;

namespace mpi {

/**
 * Main class which serves as an entry point for analysis.
 * Class name determines checker name to specify on the command line.
 * It is created once for every translation unit.
 */
class MPIChecker : public Checker<check::ASTDecl<TranslationUnitDecl>,
                                  check::PreCall, check::EndFunction> {
public:
    // ast callback–––––––––––––––––––––––––––––––––––––––––––––––––––––––
    void checkASTDecl(const TranslationUnitDecl *tuDecl,
                      AnalysisManager &analysisManager,
                      BugReporter &bugReporter) const {
        // identify MPI variables first
        MPIVariableVisitor varVisitor{analysisManager};
        varVisitor.TraverseTranslationUnitDecl(
            const_cast<TranslationUnitDecl *>(tuDecl));

        // traverse translation unit ast
        TranslationUnitVisitor tuVisitor{bugReporter, *this, analysisManager};
        tuVisitor.TraverseTranslationUnitDecl(
            const_cast<TranslationUnitDecl *>(tuDecl));

        // check after tu traversal
        tuVisitor.checkerAST_.checkPointToPointSchema();
        tuVisitor.checkerAST_.checkReachbility();

        // clear after every translation unit
        MPIRank::variables.clear();
        MPIProcessCount::variables.clear();
        MPIRankCase::cases.clear();
    }

    // path sensitive callbacks––––––––––––––––––––––––––––––––––––––––––––
    // void checkPreStmt(const CallExpr *callExpr, CheckerContext &ctx) const {
    // dynamicInit(ctx);
    // checkerSens_->checkWaitUsage(callExpr, ctx);
    // checkerSens_->checkDoubleNonblocking(callExpr, ctx);
    // }

    void checkPreCall(const CallEvent &callEvent, CheckerContext &ctx) const {
        dynamicInit(ctx);
        checkerSens_->checkWaitUsage(callEvent, ctx);
        checkerSens_->checkDoubleNonblocking(callEvent, ctx);

        // if (callEvent.getNumArgs() == 2) {
        // llvm::outs() << "log this" << "\n";
        // // callEvent.geta
        // SVal arg = callEvent.getArgSVal(0);
        // callEvent.getArgSVal(0).getAsRegion();
        // // arg.
        // // callEvent.getDecl()->dumpColor()
        // }
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

// registers the checker for static analysis.
void ento::registerMPIChecker(CheckerManager &mgr) {
    mgr.registerChecker<mpi::MPIChecker>();
}
