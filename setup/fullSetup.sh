#!/usr/bin/env zsh

:<<'_'
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
_

# script to download, config and build llvm, clang and mpi-checker

# check for internet connectivity
ping -c 1 www.google.com
if [[ $? -eq 0 ]]; then
    echo "internet connectivity established"

    llvm='http://llvm.org/svn/llvm-project'
    # choose version: release|trunk
    version='branches/release_37'
    # version='trunk'

    echo "–––––––––––––LLVM––––––––––––––"
    #download –––––––––––––––––––––––––––––––––––––––––––––––––––––––
    # contains seperated source and build folder
    mkdir llvm37
    cd llvm37

    # llvm
    svn co $llvm/llvm/$version/ repo

    # tools
    tools='repo/tools'
    svn co $llvm/cfe/$version/ $tools/clang

    # projects
    projects='repo/projects'
    svn co $llvm/compiler-rt/$version/ $projects/compiler-rt
    svn co $llvm/libcxx/$version/ $projects/libcxx

    echo "––––––––––MPI-Checker––––––––––––"
    cd repo/tools/clang/lib/StaticAnalyzer/Checkers
    git clone https://github.com/0ax1/MPI-Checker.git

    #config ––––––––––––––––––––––––––––––––––––––––––––––––––––––––
    # pipe checker registration
    cat MPI-Checker/setup/checkerTd.txt >> Checkers.td

    # use gsed if available (osx, homebrew, gnu-sed)
    sed=sed
    hash gsed 2>/dev/null
    if [[ $? -eq 0 ]]; then
        sed=gsed
    fi

    # add sources to cmake
    lineNo=$(grep -nr add_clang_library CMakeLists.txt | sed -E 's/[^0-9]*//g')

    $sed -i "${lineNo}i FILE (GLOB MPI-CHECKER MPI-Checker/src/*.cpp)" \
        CMakeLists.txt
    ((lineNo += 2))
    $sed -i "${lineNo}i \ \ \${MPI-CHECKER}" CMakeLists.txt


    abspath() {
        [[ $1 = /* ]] && echo "$1" || echo "$PWD/${1#./}"
    }
    # symlink integration tests
    ln -s `abspath MPI-Checker/tests/integration_tests/MPICheckerTest.c` \
        `abspath ../../../test/Analysis/MPICheckerTest.c`

    # symlink unit tests
    ln -s `abspath MPI-Checker/tests/unit_tests` \
        `abspath ../../../unittests/MPI-Checker`

    echo "add_subdirectory(MPI-Checker)" >> ../../../unittests/CMakeLists.txt

    cd ../../../../../../

    #build ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
    echo "––––––––––––Build–––––––––––––––"
    # create build folder
    mkdir -p build/release
    cd build/release

    # generate ninja file
    cmake -G Ninja -DCMAKE_BUILD_TYPE=RELEASE \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
        ../../repo

    # build (ninja uses max number of available cpus by default)
    ninja
    ninja ClangUnitTests
else
    # echo as error (pipe stdout to stderr)
    echo "no internet connectivity" 1>&2
fi
