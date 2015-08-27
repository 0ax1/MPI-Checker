CMAKE_MINIMUM_REQUIRED (VERSION 3.2.1)
PROJECT (target)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_FLAGS_DEBUG "-std=c11 -Wall -Wextra")
# set binary output path
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

# mpi
find_package(MPI REQUIRED)

# analysis switch
if(${ANALYZE} MATCHES 1)
    # nesting the scan-build wrapper compiler around the mpi wrapper compiler
    # prohibits the static analysis procedure
    include_directories(SYSTEM ${MPI_C_INCLUDE_PATH})
    link_directories(${MPI_C_LINK_FLAGS})
    link_libraries(${MPI_C_LIBRARIES})
    set(CMAKE_COMPILE_FLAGS ${CMAKE_COMPILE_FLAGS} ${MPI_C_COMPILE_FLAGS})
else()
    SET(CMAKE_C_COMPILER ${MPI_C_COMPILER})
endif()

add_executable(core.x partdiff-par.c askparams.c displaymatrix-mpi.c)
