#ifndef TYPEDEFS_HPP_IFNUH41O
#define TYPEDEFS_HPP_IFNUH41O

namespace mpi {

// argument schema enums ––––––––––––––––––––––––––––––––––––––
// scope enums, but keep weak typing
namespace MPIPointToPoint {
// valid for all point to point functions
enum { kBuf, kCount, kDatatype, kRank, kTag, kComm, kRequest };
}


namespace MPICollective {
// valid for all collective functions
enum { kBuf, kCount, kDatatype, kRank, kTag, kComm, kRequest };
}

namespace MPI_Comm_rank {
enum { kComm, kRank };
}

// used for argument identification
enum class InvalidArgType { kLiteral, kVariable, kReturnType };

}  // end of namespace: mpi
#endif  // end of include guard: TYPEDEFS_HPP_IFNUH41O
