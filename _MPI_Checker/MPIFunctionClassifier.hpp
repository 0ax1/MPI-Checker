#ifndef MPIFUNCTIONCLASSIFIER_HPP_U21GIA4Y
#define MPIFUNCTIONCLASSIFIER_HPP_U21GIA4Y

#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"

namespace mpi {

class MPIFunctionClassifier {
public:
    MPIFunctionClassifier(clang::ento::AnalysisManager &analysisManager) {
        identifierInit(analysisManager);
    }
    // to enable classification of mpi-functions during analysis
    // to inspect properties of mpi functions
    bool isMPIType(const clang::IdentifierInfo *) const;
    bool isSendType(const clang::IdentifierInfo *) const;
    bool isRecvType(const clang::IdentifierInfo *) const;
    bool isBlockingType(const clang::IdentifierInfo *) const;
    bool isNonBlockingType(const clang::IdentifierInfo *) const;
    bool isPointToPointType(const clang::IdentifierInfo *) const;
    bool isCollectiveType(const clang::IdentifierInfo *) const;

    bool isMPI_Scatter(const clang::IdentifierInfo *) const;

    // bool isPointToCollType(const clang::IdentifierInfo *) const;
    // bool isCollToPointType(const clang::IdentifierInfo *) const;
    // bool isCollToCollType(const clang::IdentifierInfo *) const;


private:
    void identifierInit(clang::ento::AnalysisManager &);
    void initPointToPointIdentifiers(clang::ento::AnalysisManager &);
    void initCollectiveIdentifiers(clang::ento::AnalysisManager &);
    void initAdditionalIdentifiers(clang::ento::AnalysisManager &);

    // to enable classification of mpi-functions during analysis
    std::vector<clang::IdentifierInfo *> mpiSendTypes_;
    std::vector<clang::IdentifierInfo *> mpiRecvTypes_;

    std::vector<clang::IdentifierInfo *> mpiBlockingTypes_;
    std::vector<clang::IdentifierInfo *> mpiNonBlockingTypes_;

    std::vector<clang::IdentifierInfo *> mpiPointToPointTypes_;
    std::vector<clang::IdentifierInfo *> mpiCollectiveTypes_;

    // TODO distinction needed ?
    // std::vector<clang::IdentifierInfo *> mpiPointToCollTypes_;
    // std::vector<clang::IdentifierInfo *> mpiCollToPointTypes_;
    // std::vector<clang::IdentifierInfo *> mpiCollToCollTypes_;

    std::vector<clang::IdentifierInfo *> mpiType_;

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
        *identInfo_MPI_Wait_{nullptr};
};

}  // end of namespace: mpi

#endif  // end of include guard: MPIFUNCTIONCLASSIFIER_HPP_U21GIA4Y
