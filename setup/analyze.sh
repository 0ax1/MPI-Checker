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

# cmake–––––––––––––––––––––––––––––––––––––––––––––––

# analyze
function analyze() {
    # create build folder if not there
    if [[ ! -d build/analyse ]]; then
        mkdir -p build/analyse
    fi

    cd build/analyse
    ninja clean
    cmake ../../ \
        -G Ninja \
        -DANALYZE=1 \
        -DCMAKE_BUILD_TYPE=DEBUG \
        -DCMAKE_C_COMPILER=ccc-analyzer \
        -DCMAKE_CXX_COMPILER=c++-analyzer

    # -V flag to get html report
    # -enable-checker can be used to enable additional checkers
    # -h can be used to additionally analyze headers

    scan-build --use-analyzer `which clang` $@ -V ninja

    cd ../../
}
alias checkMPI='analyze -enable-checker lx.MPIChecker'

# debug
function cmd() {
    if [[ ! -d build/debug ]]; then
        mkdir -p build/debug
    fi
    cd build/debug
    cmake -GNinja -DCMAKE_BUILD_TYPE=DEBUG ../../
    ninja
    cd ../../
}

# release
function cmr() {
    if [[ ! -d build/release ]]; then
        mkdir -p build/release
    fi
    cd build/release
    cmake -GNinja -DCMAKE_BUILD_TYPE=RELEASE ../../
    ninja
    cd ../../
}

