#include "MPIFunctionClassifier.hpp"
#include "Utility.hpp"

using namespace clang;
using namespace ento;

namespace mpi {

// classification ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
/**
 * Initializes function identifiers. Instead of using strings,
 * indentifier-pointers are initially captured
 * to recognize functions during analysis by comparison later.
 *
 * @param current ast-context used for analysis
 */
void MPIFunctionClassifier::identifierInit(
    clang::ento::AnalysisManager &analysisManager) {
    // init function identifiers
    initPointToPointIdentifiers(analysisManager);
    initCollectiveIdentifiers(analysisManager);
    initNonCommunicationIdentifiers(analysisManager);
}

void MPIFunctionClassifier::initPointToPointIdentifiers(
    clang::ento::AnalysisManager &analysisManager) {
    ASTContext &context = analysisManager.getASTContext();

    // and copy them into the correct classification containers
    identInfo_MPI_Send_ = &context.Idents.get("MPI_Send");
    mpiSendTypes_.push_back(identInfo_MPI_Send_);
    mpiPointToPointTypes_.push_back(identInfo_MPI_Send_);
    mpiBlockingTypes_.push_back(identInfo_MPI_Send_);
    mpiType_.push_back(identInfo_MPI_Send_);
    assert(identInfo_MPI_Send_);

    identInfo_MPI_Isend_ = &context.Idents.get("MPI_Isend");
    mpiSendTypes_.push_back(identInfo_MPI_Isend_);
    mpiPointToPointTypes_.push_back(identInfo_MPI_Isend_);
    mpiNonBlockingTypes_.push_back(identInfo_MPI_Isend_);
    mpiType_.push_back(identInfo_MPI_Isend_);
    assert(identInfo_MPI_Isend_);

    identInfo_MPI_Ssend_ = &context.Idents.get("MPI_Ssend");
    mpiSendTypes_.push_back(identInfo_MPI_Ssend_);
    mpiPointToPointTypes_.push_back(identInfo_MPI_Ssend_);
    mpiBlockingTypes_.push_back(identInfo_MPI_Ssend_);
    mpiType_.push_back(identInfo_MPI_Ssend_);
    assert(identInfo_MPI_Ssend_);

    identInfo_MPI_Issend_ = &context.Idents.get("MPI_Issend");
    mpiSendTypes_.push_back(identInfo_MPI_Issend_);
    mpiPointToPointTypes_.push_back(identInfo_MPI_Issend_);
    mpiNonBlockingTypes_.push_back(identInfo_MPI_Issend_);
    mpiType_.push_back(identInfo_MPI_Issend_);
    assert(identInfo_MPI_Issend_);

    identInfo_MPI_Bsend_ = &context.Idents.get("MPI_Bsend");
    mpiSendTypes_.push_back(identInfo_MPI_Bsend_);
    mpiPointToPointTypes_.push_back(identInfo_MPI_Bsend_);
    mpiBlockingTypes_.push_back(identInfo_MPI_Bsend_);
    mpiType_.push_back(identInfo_MPI_Bsend_);
    assert(identInfo_MPI_Bsend_);

    identInfo_MPI_Ibsend_ = &context.Idents.get("MPI_Ibsend");
    mpiSendTypes_.push_back(identInfo_MPI_Ibsend_);
    mpiPointToPointTypes_.push_back(identInfo_MPI_Ibsend_);
    mpiNonBlockingTypes_.push_back(identInfo_MPI_Ibsend_);
    mpiType_.push_back(identInfo_MPI_Ibsend_);
    assert(identInfo_MPI_Ibsend_);

    identInfo_MPI_Rsend_ = &context.Idents.get("MPI_Rsend");
    mpiSendTypes_.push_back(identInfo_MPI_Rsend_);
    mpiPointToPointTypes_.push_back(identInfo_MPI_Rsend_);
    mpiBlockingTypes_.push_back(identInfo_MPI_Rsend_);
    mpiType_.push_back(identInfo_MPI_Rsend_);
    assert(identInfo_MPI_Rsend_);

    identInfo_MPI_Irsend_ = &context.Idents.get("MPI_Irsend");
    mpiSendTypes_.push_back(identInfo_MPI_Irsend_);
    mpiPointToPointTypes_.push_back(identInfo_MPI_Irsend_);
    mpiBlockingTypes_.push_back(identInfo_MPI_Irsend_);
    mpiType_.push_back(identInfo_MPI_Irsend_);
    assert(identInfo_MPI_Irsend_);

    identInfo_MPI_Recv_ = &context.Idents.get("MPI_Recv");
    mpiRecvTypes_.push_back(identInfo_MPI_Recv_);
    mpiPointToPointTypes_.push_back(identInfo_MPI_Recv_);
    mpiBlockingTypes_.push_back(identInfo_MPI_Recv_);
    mpiType_.push_back(identInfo_MPI_Recv_);
    assert(identInfo_MPI_Recv_);

    identInfo_MPI_Irecv_ = &context.Idents.get("MPI_Irecv");
    mpiRecvTypes_.push_back(identInfo_MPI_Irecv_);
    mpiPointToPointTypes_.push_back(identInfo_MPI_Irecv_);
    mpiNonBlockingTypes_.push_back(identInfo_MPI_Irecv_);
    mpiType_.push_back(identInfo_MPI_Irecv_);
    assert(identInfo_MPI_Irecv_);
}

void MPIFunctionClassifier::initCollectiveIdentifiers(
    clang::ento::AnalysisManager &analysisManager) {

    // ASTContext &context = analysisManager.getASTContext();
}

void MPIFunctionClassifier::initNonCommunicationIdentifiers(
    clang::ento::AnalysisManager &analysisManager) {

    ASTContext &context = analysisManager.getASTContext();
    // non communicating functions
    identInfo_MPI_Comm_rank_ = &context.Idents.get("MPI_Comm_rank");
    mpiType_.push_back(identInfo_MPI_Comm_rank_);
    assert(identInfo_MPI_Comm_rank_);
}

/**
 * Check if MPI send function
 */
bool MPIFunctionClassifier::isMPIType(const IdentifierInfo *identInfo) const {
    return cont::isContained(mpiType_, identInfo);
}

/**
 * Check if MPI send function
 */
bool MPIFunctionClassifier::isSendType(const IdentifierInfo *identInfo) const {
    return cont::isContained(mpiSendTypes_, identInfo);
}

/**
 * Check if MPI recv function
 */
bool MPIFunctionClassifier::isRecvType(const IdentifierInfo *identInfo) const {
    return cont::isContained(mpiRecvTypes_, identInfo);
}

/**
 * Check if MPI blocking function
 */
bool MPIFunctionClassifier::isBlockingType(
    const IdentifierInfo *identInfo) const {
    return cont::isContained(mpiBlockingTypes_, identInfo);
}

/**
 * Check if MPI nonblocking function
 */
bool MPIFunctionClassifier::isNonBlockingType(
    const IdentifierInfo *identInfo) const {
    return cont::isContained(mpiNonBlockingTypes_, identInfo);
}

/**
 * Check if MPI point to point function
 */
bool MPIFunctionClassifier::isPointToPointType(
    const IdentifierInfo *identInfo) const {
    return cont::isContained(mpiPointToPointTypes_, identInfo);
}

/**
 * Check if MPI point to collective function
 */
bool MPIFunctionClassifier::isPointToCollType(
    const IdentifierInfo *identInfo) const {
    return cont::isContained(mpiPointToCollTypes_, identInfo);
}

/**
 * Check if MPI collective to point function
 */
bool MPIFunctionClassifier::isCollToPointType(
    const IdentifierInfo *identInfo) const {
    return cont::isContained(mpiCollToPointTypes_, identInfo);
}

/**
 * Check if MPI collective to collective function
 */
bool MPIFunctionClassifier::isCollToCollType(
    const IdentifierInfo *identInfo) const {
    return cont::isContained(mpiCollToCollTypes_, identInfo);
}

}  // end of namespace: mpi
