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

#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>
#include <math.h>
#include <stdbool.h>

#define tag 21

double fnn() { return 22.22; }
int f2n() { return 22; }
int f() { return rand(); }

int rank;
int buf;
int N = 0;

typedef struct {
    int rank;
    int other;
} structWithRank;

void communicate1() {
    structWithRank swr = {.rank = 0, .other = 0};
    MPI_Comm_rank(MPI_COMM_WORLD, &swr.rank);
    MPI_Request req[2];

    if (swr.rank == 0) {
        MPI_Isend(&buf, 1, MPI_INT, rank + 1, 1, MPI_COMM_WORLD, &req[0]);
        MPI_Isend(&buf, 1, MPI_INT, rank + 1, 1, MPI_COMM_WORLD, &req[0]);

    } else if (swr.rank == 1) {
        MPI_Irecv(&buf, 1, MPI_INT, rank - 1, 1, MPI_COMM_WORLD, &req[1]);
    }

    if (swr.rank == 0 || swr.rank == 1) {
        MPI_Wait(&req[0], MPI_STATUS_IGNORE);
        MPI_Waitall(2, req, MPI_STATUS_IGNORE);
    }
}

void communicate2() {
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == 0) {
        MPI_Send(&buf, 1, MPI_DOUBLE, rank + 1, 1, MPI_COMM_WORLD);
    } else if (rank == 1) {
        MPI_Recv(&buf, 1, MPI_INT, rank - 1, 2, MPI_COMM_WORLD,
                 MPI_STATUS_IGNORE);
    }
}

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);
    MPI_Request req;
    MPI_Isend(&buf, 1, MPI_INT, rank + 1, 1, MPI_COMM_WORLD, &req);

    communicate1();
    communicate2();

    MPI_Request req2;
    MPI_Wait(&req2, MPI_STATUS_IGNORE);

    MPI_Finalize();
    return 0;
}
