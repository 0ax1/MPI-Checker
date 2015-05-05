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
    bool isPointToCollType(const clang::IdentifierInfo *) const;
    bool isCollToPointType(const clang::IdentifierInfo *) const;
    bool isCollToCollType(const clang::IdentifierInfo *) const;

private:
    void identifierInit(clang::ento::AnalysisManager &);
    void initPointToPointIdentifiers(clang::ento::AnalysisManager &);
    void initCollectiveIdentifiers(clang::ento::AnalysisManager &);
    void initNonCommunicationIdentifiers(clang::ento::AnalysisManager &);

    // to enable classification of mpi-functions during analysis
    std::vector<clang::IdentifierInfo *> mpiSendTypes_;
    std::vector<clang::IdentifierInfo *> mpiRecvTypes_;

    std::vector<clang::IdentifierInfo *> mpiBlockingTypes_;
    std::vector<clang::IdentifierInfo *> mpiNonBlockingTypes_;

    std::vector<clang::IdentifierInfo *> mpiPointToPointTypes_;
    std::vector<clang::IdentifierInfo *> mpiCollectiveTypes_;
    std::vector<clang::IdentifierInfo *> mpiPointToCollTypes_;
    std::vector<clang::IdentifierInfo *> mpiCollToPointTypes_;
    std::vector<clang::IdentifierInfo *> mpiCollToCollTypes_;
    std::vector<clang::IdentifierInfo *> mpiType_;

    clang::IdentifierInfo *identInfo_MPI_Send_{nullptr},
        *identInfo_MPI_Isend_{nullptr}, *identInfo_MPI_Recv_{nullptr},
        *identInfo_MPI_Irecv_{nullptr}, *identInfo_MPI_Ssend_{nullptr},
        *identInfo_MPI_Issend_{nullptr}, *identInfo_MPI_Bsend_{nullptr},
        *identInfo_MPI_Ibsend_{nullptr}, *identInfo_MPI_Rsend_{nullptr},
        *identInfo_MPI_Irsend_{nullptr}, *identInfo_MPI_Comm_rank_{nullptr};
};

}  // end of namespace: mpi

#endif  // end of include guard: MPIFUNCTIONCLASSIFIER_HPP_U21GIA4Y
