#ifndef MPIFUNCTIONCLASSIFIER_HPP_Q3AOUNFC
#define MPIFUNCTIONCLASSIFIER_HPP_Q3AOUNFC

#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"

namespace mpi {

class MPIFunctionClassifier {
public:
    MPIFunctionClassifier(clang::ento::AnalysisManager &analysisManager) {
        identifierInit(analysisManager);
    }

    // general identifiers–––––––––––––––––––––––––––––––––––––––––––––––––
    bool isMPIType(const clang::IdentifierInfo *) const;
    bool isBlockingType(const clang::IdentifierInfo *) const;
    bool isNonBlockingType(const clang::IdentifierInfo *) const;

    // point to point identifiers––––––––––––––––––––––––––––––––––––––––––
    bool isPointToPointType(const clang::IdentifierInfo *) const;
    bool isSendType(const clang::IdentifierInfo *) const;
    bool isRecvType(const clang::IdentifierInfo *) const;

    // collective identifiers––––––––––––––––––––––––––––––––––––––––––––––
    bool isCollectiveType(const clang::IdentifierInfo *) const;
    bool isCollToColl(const clang::IdentifierInfo *) const;
    bool isScatterType(const clang::IdentifierInfo *) const;
    bool isGatherType(const clang::IdentifierInfo *) const;
    bool isAllgatherType(const clang::IdentifierInfo *) const;
    bool isAlltoallType(const clang::IdentifierInfo *) const;
    bool isReduceType(const clang::IdentifierInfo *) const;
    bool isBcastType(const clang::IdentifierInfo *) const;

    // additional identifiers ––––––––––––––––––––––––––––––––––––––––––––––
    bool isMPI_Comm_rank(const clang::IdentifierInfo *) const;
    bool isMPI_Wait(const clang::IdentifierInfo *) const;
    bool isMPI_Waitall(const clang::IdentifierInfo *) const;
    bool isWaitType(const clang::IdentifierInfo *) const;

private:

    void identifierInit(clang::ento::AnalysisManager &);
    void initPointToPointIdentifiers(clang::ento::AnalysisManager &);
    void initCollectiveIdentifiers(clang::ento::AnalysisManager &);
    void initAdditionalIdentifiers(clang::ento::AnalysisManager &);

    // to enable classification of mpi-functions during analysis
    llvm::SmallVector<clang::IdentifierInfo *, 8> mpiSendTypes_;
    llvm::SmallVector<clang::IdentifierInfo *, 2> mpiRecvTypes_;

    llvm::SmallVector<clang::IdentifierInfo *, 12> mpiBlockingTypes_;
    llvm::SmallVector<clang::IdentifierInfo *, 12> mpiNonBlockingTypes_;

    llvm::SmallVector<clang::IdentifierInfo *, 10> mpiPointToPointTypes_;
    llvm::SmallVector<clang::IdentifierInfo *, 16> mpiCollectiveTypes_;

    llvm::SmallVector<clang::IdentifierInfo *, 4> mpiPointToCollTypes_;
    llvm::SmallVector<clang::IdentifierInfo *, 4> mpiCollToPointTypes_;
    llvm::SmallVector<clang::IdentifierInfo *, 6> mpiCollToCollTypes_;

    llvm::SmallVector<clang::IdentifierInfo *, 32> mpiType_;

    // point to point functions
    clang::IdentifierInfo *identInfo_MPI_Send_{nullptr},
        *identInfo_MPI_Isend_{nullptr}, *identInfo_MPI_Ssend_{nullptr},
        *identInfo_MPI_Issend_{nullptr}, *identInfo_MPI_Bsend_{nullptr},
        *identInfo_MPI_Ibsend_{nullptr}, *identInfo_MPI_Rsend_{nullptr},
        *identInfo_MPI_Irsend_{nullptr}, *identInfo_MPI_Recv_{nullptr},
        *identInfo_MPI_Irecv_{nullptr};

    // collective functions
    clang::IdentifierInfo *identInfo_MPI_Scatter_{nullptr},
        *identInfo_MPI_Iscatter_{nullptr}, *identInfo_MPI_Gather_{nullptr},
        *identInfo_MPI_Igather_{nullptr}, *identInfo_MPI_Allgather_{nullptr},
        *identInfo_MPI_Iallgather_{nullptr}, *identInfo_MPI_Bcast_{nullptr},
        *identInfo_MPI_Ibcast_{nullptr}, *identInfo_MPI_Reduce_{nullptr},
        *identInfo_MPI_Ireduce_{nullptr}, *identInfo_MPI_Allreduce_{nullptr},
        *identInfo_MPI_Iallreduce_{nullptr}, *identInfo_MPI_Alltoall_{nullptr},
        *identInfo_MPI_Ialltoall_{nullptr}, *identInfo_MPI_Barrier_{nullptr};

    // additional functions
    clang::IdentifierInfo *identInfo_MPI_Comm_rank_{nullptr},
        *identInfo_MPI_Wait_{nullptr}, *identInfo_MPI_Waitall_{nullptr};
};

}  // end of namespace: mpi

#endif  // end of include guard: MPIFUNCTIONCLASSIFIER_HPP_Q3AOUNFC
