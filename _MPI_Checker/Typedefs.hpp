#ifndef TYPEDEFS_HPP_IFNUH41O
#define TYPEDEFS_HPP_IFNUH41O

// scope enums, but keep weak typing
// MPI_Send: buf, count, datatype, dest, tag, comm
namespace MPI_Send_idx {
enum { kBuf, kCount, kDatatype, kDest, kTag, kComm };
}
// captures potentially non const args
std::vector<unsigned char> MPI_Send_varCommArgs{
    MPI_Send_idx::kCount, MPI_Send_idx::kDest, MPI_Send_idx::kTag};


// MPI_Recv: buf, count, datatype, source, tag, comm, status
namespace MPI_Recv_idx {
enum { kBuf, kCount, kDatatype, kSource, kTag, kComm, kStatus };
}
std::vector<unsigned char> MPI_Recv_varCommArgs{
    MPI_Recv_idx::kCount, MPI_Recv_idx::kSource, MPI_Recv_idx::kTag};


// MPI_Isend: buf, count, datatype, dest, tag, comm, request
namespace MPI_Isend_idx {
enum { kBuf, kCount, kDatatype, kDest, kTag, kComm, kRequest };
}
std::vector<unsigned char> MPI_Isend_varCommArgs{
    MPI_Isend_idx::kCount, MPI_Isend_idx::kDest, MPI_Isend_idx::kTag};


// MPI_Irecv: buf, count, datatype, source, tag, comm, request
namespace MPI_Irecv_idx {
enum { kBuf, kCount, kDatatype, kSource, kTag, kComm, kRequest };
}
std::vector<unsigned char> MPI_Irecv_varCommArgs{
    MPI_Irecv_idx::kCount, MPI_Irecv_idx::kSource, MPI_Irecv_idx::kTag};

#endif  // end of include guard: TYPEDEFS_HPP_IFNUH41O
