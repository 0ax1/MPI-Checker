#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>
#include <math.h>
#include <stdbool.h>

#define tag 21

double fnn() { return 22.22; }
int f2n() { return 22; }

int rank = 0;
int buf;
int N;

int f() { return rand(); }

void communicate1() {
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 1) {
    } else if (rank == 2) {
    } else {
        MPI_Request sendReq1;
        MPI_Request recvReq1;

        MPI_Isend(&buf, 1, MPI_DOUBLE, f() + N + 3 + rank + 1, 0, MPI_COMM_WORLD,
                  &sendReq1);
        MPI_Irecv(&buf, 1, MPI_INT, N + f() + 3 + rank - 1, 0, MPI_COMM_WORLD,
                  &recvReq1);

        MPI_Request r[2] = {sendReq1, recvReq1};
        MPI_Waitall(2, r, MPI_STATUSES_IGNORE);

        MPI_Bcast(&N, 2, MPI_INT, 0 , MPI_COMM_WORLD);
        MPI_Bcast(&N, 2, MPI_INT, 0 , MPI_COMM_WORLD);

        MPI_Wait(&recvReq1, MPI_STATUS_IGNORE);
    }
}

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);
    communicate1();

    MPI_Finalize();
    return 0;
}
