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
