#!/usr/bin/env zsh

# script to setup mpi-checker

# check for internet connectivity
ping -c 1 www.google.com
if [[ $? -eq 0 ]]; then
    echo "internet connectivity established"

    echo "––––––––––MPI-Checker––––––––––––"
    #download –––––––––––––––––––––––––––––––––––––––––––––––––––––––
    # clone mpi-checker project
    cd tools/clang/lib/StaticAnalyzer/Checkers
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

else
    # echo as error (pipe stdout to stderr)
    echo "no internet connectivity" 1>&2
fi
