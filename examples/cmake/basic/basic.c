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

int rank;
int buf;
int N = 0;

typedef struct {
    int rank;
}basic_t ;

int f() { return rand(); }

void communicate1() {
    MPI_Request req1, req2;

    basic_t bt = {.rank = 0};
    printf("%i\n", bt.rank);
    printf("%i\n", bt.rank);
    printf("%i\n", bt.rank);

    MPI_Comm_rank(MPI_COMM_WORLD, &bt.rank);

    if (bt.rank == 0) {
        MPI_Isend(&buf, 1, MPI_INT, rank + 1, 0, MPI_COMM_WORLD, &req1);
        MPI_Isend(&buf, 1, MPI_DOUBLE, rank + 1, 1, MPI_COMM_WORLD, &req2);

        MPI_Request r[2] = {req1, req2};
        MPI_Waitall(2, r, MPI_STATUSES_IGNORE);

    } else if (bt.rank == 1) {

        MPI_Irecv(&buf, 1, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, &req1);
        MPI_Irecv(&buf, 1, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, &req2);

        MPI_Request r[2] = {req1, req2};
        MPI_Waitall(2, r, MPI_STATUSES_IGNORE);

        MPI_Wait(&req1, MPI_STATUS_IGNORE);
    }
}

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);
    communicate1();

    MPI_Finalize();
    return 0;
}
