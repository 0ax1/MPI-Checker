CMAKE_MINIMUM_REQUIRED (VERSION 3.2.1)
PROJECT (target)

set(CMAKE_C_FLAGS_DEBUG "-std=c11 -Wall -Wextra")
# set binary output path
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

# mpi
find_package(MPI REQUIRED)
SET(CMAKE_C_COMPILER ${MPI_C_COMPILER})

add_executable(core.x basic.c)
