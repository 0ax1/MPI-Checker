// RUN: %clang_cc1 -analyze -analyzer-checker=lx.MPIChecker -verify %s

#include </usr/local/include/mpi.h>

void doubleWait() {
    int rank = 0;
    double buf = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 1) {
    } else if (rank == 2) {
    } else {
        MPI_Request sendReq1;
        MPI_Request recvReq1;

        MPI_Isend(&buf, 1, MPI_DOUBLE, rank + 1, 0, MPI_COMM_WORLD, &sendReq1);
        MPI_Irecv(&buf, 1, MPI_DOUBLE, rank - 1, 0, MPI_COMM_WORLD, &recvReq1);
        MPI_Request r[2] = {sendReq1, recvReq1};
        MPI_Waitall(2, r, MPI_STATUSES_IGNORE);
        MPI_Wait(&recvReq1, MPI_STATUS_IGNORE); // expected-warning{{Request recvReq1 is already waited upon by MPI_Waitall in line 18.}}
    }
}

void missingWait() {
    int rank = 0;
    double buf = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 1) {
    } else {
        MPI_Request sendReq1;
        MPI_Request recvReq1;

        MPI_Isend(&buf, 1, MPI_DOUBLE, rank + 1, 0, MPI_COMM_WORLD, &sendReq1); // expected-warning{{Nonblocking call using request sendReq1 has no matching wait. }}
        MPI_Irecv(&buf, 1, MPI_DOUBLE, rank - 1, 0, MPI_COMM_WORLD, &recvReq1);
        MPI_Wait(&recvReq1, MPI_STATUS_IGNORE);
    }
}

void doubleNonblocking() {
    int rank = 0;
    double buf = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 1) {
    } else {
        MPI_Request sendReq1;
        MPI_Request recvReq1;

        MPI_Isend(&buf, 1, MPI_DOUBLE, rank + 1, 0, MPI_COMM_WORLD, &sendReq1);
        MPI_Irecv(&buf, 1, MPI_DOUBLE, rank - 1, 0, MPI_COMM_WORLD, &sendReq1); // expected-warning{{Request sendReq1 is already in use by nonblocking call MPI_Isend in line 47. }}
        MPI_Wait(&sendReq1, MPI_STATUS_IGNORE);
    }
}

void missingNonBlocking() {
    int rank = 0;
    double buf = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 1) {
        MPI_Request sendReq1;
        MPI_Wait(&sendReq1, MPI_STATUS_IGNORE); // expected-warning{{Request sendReq1 has no matching nonblocking call.}}
    }
}
