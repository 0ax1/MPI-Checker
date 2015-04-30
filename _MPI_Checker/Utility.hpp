#ifndef UTILITY_HPP_SVQZWTL8
#define UTILITY_HPP_SVQZWTL8

#include <sstream>
#include <vector>

namespace lx {
namespace util {

std::vector<std::string> &split(const std::string &, char,
                                    std::vector<std::string> &);
std::vector<std::string> split(const std::string &, char);
}  // end of namespace: util
}  // end of namespace: lx

#endif  // end of include guard: UTILITY_HPP_SVQZWTL8
