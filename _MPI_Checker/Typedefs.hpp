#ifndef TYPEDEFS_HPP_IFNUH41O
#define TYPEDEFS_HPP_IFNUH41O

// argument schema enums ––––––––––––––––––––––––––––––––––––––
// scope enums, but keep weak typing
namespace MPIPointToPoint {
// valid for all point to point functions
enum { kBuf, kCount, kDatatype, kRank, kTag, kComm, kRequest };
}

namespace MPI_Comm_rank {
enum { kComm, kRank };
}

// used for argument identification
enum class FloatArgType { kLiteral, kVariable, kReturnType };

#endif  // end of include guard: TYPEDEFS_HPP_IFNUH41O
