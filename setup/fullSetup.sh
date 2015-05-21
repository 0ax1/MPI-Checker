#!/usr/bin/env zsh

# script to download, config and build llvm, clang and mpi-checker

# check for internet connectivity
ping -c 1 www.google.com
if [[ $? -eq 0 ]]; then
    echo "internet connectivity established"

    echo "–––––––––––––LLVM––––––––––––––"
    #download –––––––––––––––––––––––––––––––––––––––––––––––––––––––
    # contains seperated source and build folder
    mkdir llvm36
    cd llvm36

    # init llvm-architecture repo(sitory) folder
    svn co http://llvm.org/svn/llvm-project/llvm/branches/release_36/ repo

    # tools folder
    cd repo/tools
    svn co http://llvm.org/svn/llvm-project/cfe/branches/release_36/ clang

    # clone mpi-checker project
    cd clang/lib/StaticAnalyzer/Checkers
    git clone https://github.com/0ax1/MPI-Checker.git

    echo "––––––––––MPI-Checker––––––––––––"
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

    cd ../../../../../../

    #build ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
    # create build folder
    mkdir -p build/release
    cd build/release

    # generate ninja file
    cmake -G Ninja -DCMAKE_BUILD_TYPE=RELEASE \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
        ../../repo

    # build (ninja uses max number of available cpus by default)
    ninja
else
    # echo as error (pipe stdout to stderr)
    echo "no internet connectivity" 1>&2
fi
