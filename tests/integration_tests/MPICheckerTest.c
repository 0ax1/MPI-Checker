// RUN: %clang_cc1 -I/usr/local/include/ -analyze -analyzer-checker=lx.MPIChecker -verify %s

// add the mpi include path to the first line with -I/...
// if mpi.h is not found

// clang -cc1 uses the compiler frontend
// without the compiler driver.

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

#include <mpi.h>

/*
 * Perform black box "mid-level integration testing"
 * by means of example functions.
 * http://clang-developers.42468.n3.nabble.com/unit-tests-for-Clang-TDD-td3423630.html
 */

void doubleWait() {
    int rank = 0;
    double buf = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0) {
    } else {
        MPI_Request sendReq1,  recvReq1;

        MPI_Isend(&buf, 1, MPI_DOUBLE, rank + 1, 0, MPI_COMM_WORLD, &sendReq1);
        MPI_Irecv(&buf, 1, MPI_DOUBLE, rank - 1, 0, MPI_COMM_WORLD, &recvReq1);
        MPI_Request r[2] = {sendReq1, recvReq1};
        MPI_Waitall(2, r, MPI_STATUSES_IGNORE);
        MPI_Wait(&recvReq1, MPI_STATUS_IGNORE); // expected-warning{{Request recvReq1 is already waited upon by MPI_Waitall in line 52.}}
    }
}

void doubleWait2() {
    int rank = 0;
    double buf = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 1) {
        MPI_Request sendReq1,  recvReq1;

        MPI_Isend(&buf, 1, MPI_DOUBLE, rank + 1, 0, MPI_COMM_WORLD, &sendReq1);
        MPI_Irecv(&buf, 1, MPI_DOUBLE, rank - 1, 0, MPI_COMM_WORLD, &recvReq1);
        MPI_Wait(&sendReq1, MPI_STATUS_IGNORE);
        MPI_Wait(&recvReq1, MPI_STATUS_IGNORE);
        MPI_Wait(&recvReq1, MPI_STATUS_IGNORE); // expected-warning{{Request recvReq1 is already waited upon by MPI_Wait in line 67.}}
    }
}

void missingWait() {
    int rank = 0;
    double buf = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 1) {
    } else {
        MPI_Request sendReq1,  recvReq1;

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
        MPI_Request sendReq1,  recvReq1;

        MPI_Isend(&buf, 1, MPI_DOUBLE, rank + 1, 0, MPI_COMM_WORLD, &sendReq1);
        MPI_Irecv(&buf, 1, MPI_DOUBLE, rank - 1, 0, MPI_COMM_WORLD, &sendReq1); // expected-warning{{Request sendReq1 is already in use by nonblocking call MPI_Isend in line 94. }}
        MPI_Wait(&sendReq1, MPI_STATUS_IGNORE);
    }
}

void doubleNonblocking2() {
    int rank = 0;
    double buf = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    MPI_Request req;
    MPI_Ireduce(MPI_IN_PLACE, &buf, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD, &req);
    MPI_Ireduce(MPI_IN_PLACE, &buf, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD, &req); // expected-warning{{Request req is already in use by nonblocking call MPI_Ireduce in line 106. }}
    MPI_Wait(&req, MPI_STATUS_IGNORE);
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
