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
 * Deletes all appearances of given element.
 *
 * @param container
 * @param elementToErase
 */
template <typename T, typename E>
void eraseAll(T &container, E &&elementToErase) {
    container.erase(
        std::remove(container.begin(), container.end(), elementToErase),
        container.end());
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
    std::copy(source.begin(), source.end(), std::back_inserter(dest));
}

/**
 * Find an element by predicate. Return an iterator.
 *
 * @param container
 * @param predicate
 * @return iterator
 */
template <typename T, typename P>
typename T::iterator findPred(T &cont, P pred) {
    return std::find_if(cont.begin(), cont.end(), pred);
}

/**
 * Checks if two containers are permutations of each other.
 *
 * @tparam T1 type of container 1
 * @tparam T2 type of container 2
 * @param first container
 * @param second container
 *
 * @return isPermutation
 */
template <typename T1, typename T2>
bool isPermutation(const T1 &first, const T2 &second) {
    // size must match
    if (first.size() != second.size()) return false;

    // copy because matches get erased
    auto copy = first;
    for (auto &componentFromSecond : second) {
        if (!cont::isContained(copy, componentFromSecond)) {
            return false;
        } else {
            // to omit double matching
            cont::erase(copy, componentFromSecond);
        }
    }
    return true;
}

}  // end of namespace: cont

#endif  // end of include guard: CONTAINER_HPP_XM1FDRVJ
