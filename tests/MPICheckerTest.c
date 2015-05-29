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

void missingReceive() {
    int rank = 0;
    double buf = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0) {
        MPI_Send(&buf, 1, MPI_DOUBLE, rank + 1, 0, MPI_COMM_WORLD); // expected-warning{{No matching receive function found.}}
    }
}

void missingSend() {
    int rank = 0;
    double buf = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0) {
        MPI_Recv(&buf, 1, MPI_DOUBLE, rank + 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE); // expected-warning{{No matching send function found.}}
    }
}

void typeMismatch() {
    int rank = 0;
    double buf = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0) {
        MPI_Send(&buf, 1, MPI_INT, rank + 1, 0, MPI_COMM_WORLD); // expected-warning{{Buffer type and specified MPI type do not match. }}
    }
    else if (rank == 1) {
        MPI_Recv(&buf, 1, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE); // expected-warning{{Buffer type and specified MPI type do not match. }}
    }
}

void collectiveInBranch() {
    int rank = 0;
    double buf = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0) {
        float global_sum;
        MPI_Reduce(MPI_IN_PLACE, &global_sum, 1, MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD); // expected-warning{{Collective calls must be executed by all processes. Move this call out of the rank branch. }}
    }
}

void invalidArgTypeLiteral() {
    int rank = 0;
    int buf = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0) {
        MPI_Send(&buf, 1, MPI_INT, rank + 1.1, 0, MPI_COMM_WORLD); // expected-warning{{Literal type used at index 3 is not valid.}}
    }
    else if (rank == 1) {
        MPI_Recv(&buf, 1, MPI_INT, rank - 1.1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE); // expected-warning{{Literal type used at index 3 is not valid.}}
    }
}

double doubleVal() {
    return 1.1;
}
void invalidReturnType() {
    int rank = 0;
    int buf = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0) {
        MPI_Send(&buf, 1, MPI_INT, rank + doubleVal(), 0, MPI_COMM_WORLD); // expected-warning{{Return value type used at index 3 is not valid.}}
    }
    else if (rank == 1) {
        MPI_Recv(&buf, 1, MPI_INT, rank - doubleVal(), 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE); // expected-warning{{Return value type used at index 3 is not valid.}}
    }
}

void unreachableCall() {
    int rank = 0;
    int buf = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0) {
        MPI_Send(&buf, 1, MPI_INT, rank + 1, 0, MPI_COMM_WORLD);
        MPI_Recv(&buf, 1, MPI_INT, rank + 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE); // expected-warning{{Call is not reachable. Schema leads to a deadlock.}}
    }
    else if (rank == 1) {
        MPI_Send(&buf, 1, MPI_INT, rank - 1, 0, MPI_COMM_WORLD);
        MPI_Recv(&buf, 1, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE); // expected-warning{{Call is not reachable. Schema leads to a deadlock.}}
    }
}
