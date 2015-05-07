#ifndef CONTAINER_HPP_XM1FDRVJ
#define CONTAINER_HPP_XM1FDRVJ

#include <algorithm>

namespace cont {

// convenience wrapper for container functions

/**
 * Check if given element is contained.
 *
 * @param container
 * @param elementToCheck
 *
 * @return isElementContained
 */
template <typename T, typename E>
bool isContained(const T &container, const E &elementToCheck) {
    return std::find(container.begin(), container.end(), elementToCheck) !=
           container.end();
}

/**
 * Check if given element is contained.
 *
 * @param container
 * @param elementToCheck
 *
 * @return isElementContained
 */
template <typename T, typename P>
bool isContainedPred(const T &container, P predicate) {
    return std::find_if(container.begin(), container.end(), predicate) !=
           container.end();
}

/**
 * Deletes first appearance of given element.
 *
 * @param container
 * @param elementToErase
 */
template <typename T, typename E>
void erase(T &container, E &elementToErase) {
    auto it = std::find(container.begin(), container.end(), elementToErase);
    if (it != container.end()) container.erase(it);
}

/**
 * Deletes first appearance of given pointer.
 *
 * @param container
 * @param elementToErase
 */
template <typename T, typename E>
void erasePtr(T &container, E elementToErase) {
    auto it = std::find(container.begin(), container.end(), elementToErase);
    if (it != container.end()) container.erase(it);
}

/**
 * Deletes first element that matches the predicate.
 *
 * @param container
 * @param predicate
 */
template <typename T, typename P>
void erasePred(T &container, P predicate) {
    auto it = std::find_if(container.begin(), container.end(), predicate);
    if (it != container.end()) container.erase(it);
}

/**
 * Deletes element at given index.
 *
 * @param container
 * @param index
 */
template <typename T>
void eraseIndex(T &container, size_t idx) {
    container.erase(container.begin() + idx);
}

/**
 * Sort with default criterion.
 *
 * @param container
 */
template <typename T>
void sort(T &container) {
    std::sort(container.begin(), container.end());
}

/**
 * Sort with given predicate.
 *
 * @param container
 * @param predicate
 */
template <typename T, typename P>
void sortPred(T &container, P predicate) {
    std::sort(container.begin(), container.end(), predicate);
}

/**
 * Get index for element in container.
 *
 * @param container
 * @param element
 */
template <typename T, typename E>
size_t index(const T &container, const E &element) {
    return std::find(container.begin(), container.end(), element) -
           container.begin();
}

/**
 * Get index for predicate.
 *
 * @param container
 * @param predicate
 */
template <typename T, typename P>
size_t indexPred(const T &container, P predicate) {
    return std::find_if(container.begin(), container.end(), predicate) -
           container.begin();
}

/**
 * Copy elements from one container to another.
 *
 * @param source
 * @param dest
 */
template <typename T, typename T2>
void copy(const T &source, T2 &dest) {
      std::copy(source.begin(), source.end(), dest.begin());
}

}  // end of namespace: cont

#endif  // end of include guard: CONTAINER_HPP_XM1FDRVJ
