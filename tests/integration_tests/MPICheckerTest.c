// RUN: %clang_cc1 -I/usr/include/ -I/usr/local/include/ -analyze -analyzer-checker=lx.MPIChecker -verify %s

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
#include <complex.h>

/*
 * Perform black box "mid-level integration testing".
 * Calls between different functions should not be matched -> set tags.
 */

void doubleWait() {
    int rank = 0;
    double buf = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0) {
    } else {
        MPI_Request sendReq1,  recvReq1;

        MPI_Isend(&buf, 1, MPI_DOUBLE, rank + 1, 1, MPI_COMM_WORLD, &sendReq1);
        MPI_Irecv(&buf, 1, MPI_DOUBLE, rank - 1, 1, MPI_COMM_WORLD, &recvReq1);
        MPI_Request r[2] = {sendReq1, recvReq1};
        MPI_Waitall(2, r, MPI_STATUSES_IGNORE);
        MPI_Wait(&recvReq1, MPI_STATUS_IGNORE); // expected-warning{{Request recvReq1 is already waited upon by MPI_Waitall in line 52.}}
    }
}

void doubleWait2() {
    int rank = 0;
    double buf = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank != 0) {
        MPI_Request sendReq1,  recvReq1;

        MPI_Isend(&buf, 1, MPI_DOUBLE, rank + 1, 2, MPI_COMM_WORLD, &sendReq1);
        MPI_Irecv(&buf, 1, MPI_DOUBLE, rank - 1, 2, MPI_COMM_WORLD, &recvReq1);
        MPI_Wait(&sendReq1, MPI_STATUS_IGNORE);
        MPI_Wait(&recvReq1, MPI_STATUS_IGNORE);
        MPI_Wait(&recvReq1, MPI_STATUS_IGNORE); // expected-warning{{Request recvReq1 is already waited upon by MPI_Wait in line 67.}}
    }
}

void missingWait() {
    int rank = 0;
    double buf = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0) {
    } else {
        MPI_Request sendReq1,  recvReq1;

        MPI_Isend(&buf, 1, MPI_DOUBLE, rank + 1, 3, MPI_COMM_WORLD, &sendReq1); // expected-warning{{Nonblocking call using request sendReq1 has no matching wait. }}
        MPI_Irecv(&buf, 1, MPI_DOUBLE, rank - 1, 3, MPI_COMM_WORLD, &recvReq1);
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

        MPI_Isend(&buf, 1, MPI_DOUBLE, rank + 1, 4, MPI_COMM_WORLD, &sendReq1);
        MPI_Irecv(&buf, 1, MPI_DOUBLE, rank - 1, 4, MPI_COMM_WORLD, &sendReq1); // expected-warning{{Request sendReq1 is already in use by nonblocking call MPI_Isend in line 94. }}
        MPI_Wait(&sendReq1, MPI_STATUS_IGNORE);
    }
}

void doubleNonblocking2() {
    int rank = 0;
    double buf = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    MPI_Request req;
    MPI_Ireduce(MPI_IN_PLACE, &buf, 1, MPI_DOUBLE, MPI_SUM, 5, MPI_COMM_WORLD, &req);
    MPI_Ireduce(MPI_IN_PLACE, &buf, 1, MPI_DOUBLE, MPI_SUM, 5, MPI_COMM_WORLD, &req); // expected-warning{{Request req is already in use by nonblocking call MPI_Ireduce in line 106. }}
    MPI_Wait(&req, MPI_STATUS_IGNORE);
}

void missingNonBlocking() {
    int rank = 0;
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
        MPI_Send(&buf, 1, MPI_DOUBLE, rank + 1, 6, MPI_COMM_WORLD); // expected-warning{{No matching receive function found. }}
    }
}

void missingSend() {
    int rank = 0;
    double buf = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0) {
        MPI_Recv(&buf, 1, MPI_DOUBLE, rank + 1, 7, MPI_COMM_WORLD, MPI_STATUS_IGNORE); // expected-warning{{No matching send function found.}}
    }
}

void matchedPartner1() {
    int rank = 0;
    double buf = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0) {
        MPI_Send(&buf, 1, MPI_DOUBLE, rank + 1, 24, MPI_COMM_WORLD);
    } else {
        MPI_Recv(&buf, 1, MPI_DOUBLE, rank - 1, 24, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
} // no errors

void unmatchedPartner1() {
    int rank = 0;
    double buf = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0) {
        MPI_Send(&buf, 1, MPI_DOUBLE, rank + 1, 25, MPI_COMM_WORLD); // expected-warning{{No matching receive function found. }}
    } else {
        MPI_Recv(&buf, 1, MPI_DOUBLE, rank + 1, 25, MPI_COMM_WORLD, MPI_STATUS_IGNORE); // expected-warning{{No matching send function found.}}
    }
} // receive should be rank - 1

void matchedPartner2() {
    int rank = 0;
    double buf = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank > 0) {
        MPI_Send(&buf, 1, MPI_DOUBLE, rank + 1, 26, MPI_COMM_WORLD);
    } else {
        MPI_Recv(&buf, 1, MPI_DOUBLE, rank - 1, 26, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
} // no errors

void unmatchedPartner2() {
    int rank = 0;
    double buf = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Request sendReq;
    if (rank > 0) {
        MPI_Isend(&buf, 1, MPI_DOUBLE, rank + 1, 27, MPI_COMM_WORLD, &sendReq); // expected-warning{{No matching receive function found. }}
        MPI_Wait(&sendReq, MPI_STATUS_IGNORE);
    }
    else {
        MPI_Recv(&buf, 2, MPI_DOUBLE, rank - 1, 27, MPI_COMM_WORLD, MPI_STATUS_IGNORE); // expected-warning{{No matching send function found.}}
    }
}

void matchedPartner3() {
    int rank = 0;
    double buf = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Request sendReq;
    if (rank < 10) {
        MPI_Isend(&buf, 1, MPI_DOUBLE, rank + 1, 28, MPI_COMM_WORLD, &sendReq);
        MPI_Recv(&buf, 1, MPI_DOUBLE, rank - 1, 28, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Wait(&sendReq, MPI_STATUS_IGNORE);
    }
} // no error

void unmatchedPartner3() {
    int rank = 0;
    double buf = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Request sendReq, recvReq;
    if (rank == 10) {
        MPI_Isend(&buf, 1, MPI_DOUBLE, rank + 1, 29, MPI_COMM_WORLD, &sendReq); // expected-warning{{No matching receive function found. }}
        MPI_Irecv(&buf, 1, MPI_DOUBLE, rank - 1, 29, MPI_COMM_WORLD, &recvReq); // expected-warning{{No matching send function found.}}
        MPI_Wait(&sendReq, MPI_STATUS_IGNORE);
        MPI_Wait(&recvReq, MPI_STATUS_IGNORE);
    }
} // rank case cannot do p2p communication with itself

void matchedPartner4() {
    int rank = 0;
    double buf = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Request sendReq;
    if (rank > 0) {
        MPI_Isend(&buf, 1, MPI_DOUBLE, rank + 1, 30, MPI_COMM_WORLD, &sendReq);
        MPI_Wait(&sendReq, MPI_STATUS_IGNORE);
        MPI_Recv(&buf, 1, MPI_DOUBLE, rank - 1, 30, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    } else {
        MPI_Isend(&buf, 1, MPI_DOUBLE, rank + 1, 30, MPI_COMM_WORLD, &sendReq);
        MPI_Wait(&sendReq, MPI_STATUS_IGNORE);
        MPI_Recv(&buf, 1, MPI_DOUBLE, rank - 1, 30, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
} // no error

void unmatchedPartner4() {
    int rank = 0;
    int var = 1;
    int var2 = 1;
    double buf = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Request sendReq;
    if (rank > 0) {
        MPI_Send(&buf, var + 1 - var2, MPI_DOUBLE, var + 11 + rank + 1, 31, MPI_COMM_WORLD);// expected-warning{{No matching receive function found. }}
    } else {
        MPI_Recv(&buf, var2 + 1 - var, MPI_DOUBLE, 11 + var + rank - 1, 31, MPI_COMM_WORLD, MPI_STATUS_IGNORE);// expected-warning{{No matching send function found.}}
    }
} // if subtractions are used operands must appear in the same order (var + 1 - var2 != var2 + 1 - var)

void matchedPartner5() {
    int rank = 0;
    int var = 1;
    int var2 = 1;
    double buf = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Request sendReq;
    if (rank > 0) {
        MPI_Send(&buf, var + 1 + var2, MPI_DOUBLE, var + 11 + rank + 1, 32, MPI_COMM_WORLD);
    } else {
        MPI_Recv(&buf, var2 + 1 + var, MPI_DOUBLE, 11 + var + rank - 1, 32, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
} // no error, permutations are allowed within an argument if all operators are additions (excluding the rank +/- 1 part)

void matchedPartner6() {
    int rank = 0;
    int var = 1;
    int var2 = 1;
    double buf = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Request req;
    if (rank > 0) {
        MPI_Ssend(&buf, 1, MPI_DOUBLE, rank + 1, 33, MPI_COMM_WORLD);
        MPI_Send(&buf, 1, MPI_DOUBLE, rank + 1, 33, MPI_COMM_WORLD);
        MPI_Issend(&buf, 1, MPI_DOUBLE, rank + 1, 33, MPI_COMM_WORLD, &req);
        MPI_Wait(&req, MPI_STATUS_IGNORE);
    } else {
        MPI_Recv(&buf, 1, MPI_DOUBLE, rank - 1, 33, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Irecv(&buf, 1, MPI_DOUBLE, rank - 1, 33, MPI_COMM_WORLD, &req);
        MPI_Wait(&req, MPI_STATUS_IGNORE);
        MPI_Recv(&buf, 1, MPI_DOUBLE, rank - 1, 33, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
} // no error, different p2p types are matched

void matchFirstToLast() {
    int rankA = 0;
    int rankB = 0;
    int sizeA = 0;
    int sizeB = 0;
    double buf = 0;

    MPI_Comm_rank(MPI_COMM_WORLD, &rankA);
    MPI_Comm_rank(MPI_COMM_WORLD, &rankB);
    MPI_Comm_size(MPI_COMM_WORLD, &sizeA);
    MPI_Comm_size(MPI_COMM_WORLD, &sizeB);

    if (rankA == 0) {
        MPI_Send(&buf, 1, MPI_DOUBLE, sizeA - 1, 8, MPI_COMM_WORLD);
    } else if (sizeB - 1 == rankB) {
        MPI_Recv(&buf, 1, MPI_DOUBLE, 0, 8, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
} // no error

void firstToLastUnmatched1() {
    int rankA = 0;
    int rankB = 0;
    int sizeA = 0;
    int sizeB = 0;
    double buf = 0;

    MPI_Comm_rank(MPI_COMM_WORLD, &rankA);
    MPI_Comm_rank(MPI_COMM_WORLD, &rankB);
    MPI_Comm_size(MPI_COMM_WORLD, &sizeA);
    MPI_Comm_size(MPI_COMM_WORLD, &sizeB);

    if (rankA == 1) {
        MPI_Send(&buf, 1, MPI_DOUBLE, sizeA - 1, 9, MPI_COMM_WORLD); // expected-warning{{No matching receive function found.}}
    } else if (sizeB - 1 == rankB) {
        MPI_Recv(&buf, 1, MPI_DOUBLE, 0, 9, MPI_COMM_WORLD, MPI_STATUS_IGNORE); // expected-warning{{No matching send function found.}}
    }
}

void firstToLastUnmatched2() {
    int rankA = 0;
    int rankB = 0;
    int sizeA = 0;
    int sizeB = 0;
    double buf = 0;

    MPI_Comm_rank(MPI_COMM_WORLD, &rankA);
    MPI_Comm_rank(MPI_COMM_WORLD, &rankB);
    MPI_Comm_size(MPI_COMM_WORLD, &sizeA);
    MPI_Comm_size(MPI_COMM_WORLD, &sizeB);

    if (rankA == 0) {
        MPI_Send(&buf, 1, MPI_DOUBLE, sizeA - 2, 20, MPI_COMM_WORLD); // expected-warning{{No matching receive function found.}}
    } else if (sizeB - 1 == rankB) {
        MPI_Recv(&buf, 1, MPI_DOUBLE, 0, 20, MPI_COMM_WORLD, MPI_STATUS_IGNORE); // expected-warning{{No matching send function found.}}
    }
}

void matchLastToFirst() {
    int rankA = 0;
    int rankB = 0;
    int sizeA = 0;
    int sizeB = 0;
    double buf = 0;

    MPI_Comm_rank(MPI_COMM_WORLD, &rankA);
    MPI_Comm_rank(MPI_COMM_WORLD, &rankB);
    MPI_Comm_size(MPI_COMM_WORLD, &sizeA);
    MPI_Comm_size(MPI_COMM_WORLD, &sizeB);

    if (rankA == 0) {
        MPI_Recv(&buf, 1, MPI_DOUBLE, sizeA - 1, 10, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    } else if (sizeB - 1 == rankB) {
        MPI_Send(&buf, 1, MPI_DOUBLE, 0, 10, MPI_COMM_WORLD);
    }
} // no error

void typeMismatch1() {
    double buf = 0;
    MPI_Reduce(MPI_IN_PLACE, &buf, 1, MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD); // expected-warning{{Buffer type and specified MPI type do not match. }}
}

void typeMismatch2() {
    int buf = 0;
    MPI_Reduce(MPI_IN_PLACE, &buf, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD); // expected-warning{{Buffer type and specified MPI type do not match. }}
}

void typeMismatch3() {
    long double buf = 11;
    MPI_Reduce(MPI_IN_PLACE, &buf, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD); // expected-warning{{Buffer type and specified MPI type do not match. }}
}

void typeMismatch4() {
    long double complex buf = 11;
    MPI_Reduce(MPI_IN_PLACE, &buf, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD); // expected-warning{{Buffer type and specified MPI type do not match. }}
}

void typeMatch1() {
    double buf = 0;
    MPI_Reduce(MPI_IN_PLACE, &buf, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
} // no error

void typeMatch2() {
    char buf = '1';
    MPI_Reduce(MPI_IN_PLACE, &buf, 1, MPI_CHAR, MPI_SUM, 0, MPI_COMM_WORLD);
} // no error

void typeMatch3() {
    long double complex buf = 11;
    MPI_Reduce(MPI_IN_PLACE, &buf, 1, MPI_C_LONG_DOUBLE_COMPLEX, MPI_SUM, 0, MPI_COMM_WORLD);
} // no error

void typeMatch4() {
    float ***buf;
    ***buf = 11;
    MPI_Reduce(MPI_IN_PLACE, &buf, 1, MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD);
} // no error

void typeMatch5() {
    typedef int Int;
    Int buf = 1;
    MPI_Reduce(MPI_IN_PLACE, &buf, 1, MPI_CHAR, MPI_SUM, 0, MPI_COMM_WORLD);
} // no error, checker makes no assumptions about typedefs

void typeMatch6() {
    struct a { int x; } buf;
    MPI_Reduce(MPI_IN_PLACE, &buf, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
} // no error, checker does not validate in case of structs

void typeMatch7() {
    struct a { int x; } buf;
    MPI_Reduce(MPI_IN_PLACE, &buf, 4, MPI_BYTE, MPI_SUM, 0, MPI_COMM_WORLD);
} // no error, checker accepts any type to match MPI_BYTE
  // and makes no assumptions about the buffer size

void collectiveInBranch() {
    int rank = 0;
    int x = 22;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (11 != 22 && rank >= 0 || NULL) {
        float global_sum;
        MPI_Reduce(MPI_IN_PLACE, &global_sum, 1, MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD); // expected-warning{{Collective calls must be executed by all processes. Move this call out of the rank branch. }}
    }

    if (rank > 1 || x != 7) {
        MPI_Barrier(MPI_COMM_WORLD); // expected-warning{{Collective calls must be executed by all processes. Move this call out of the rank branch. }}
    }
}

void invalidArgTypeLiteral() {
    int rank = 0;
    int buf = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0) {
        MPI_Send(&buf, 1, MPI_INT, rank + 1.1, 17, MPI_COMM_WORLD); // expected-warning{{Literal type used at index 3 is not valid.}}
    }
    else if (rank == 1) {
        MPI_Recv(&buf, 1, MPI_INT, rank - 1.1, 17, MPI_COMM_WORLD, MPI_STATUS_IGNORE); // expected-warning{{Literal type used at index 3 is not valid.}}
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
        MPI_Send(&buf, 1, MPI_INT, rank + doubleVal(), 18, MPI_COMM_WORLD); // expected-warning{{Return value type used at index 3 is not valid.}}
    }
    else if (rank == 1) {
        MPI_Recv(&buf, 1, MPI_INT, rank - doubleVal(), 18, MPI_COMM_WORLD, MPI_STATUS_IGNORE); // expected-warning{{Return value type used at index 3 is not valid.}}
    }
}

void unreachableCall() {
    int rank = 0;
    int buf = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0) {
        MPI_Send(&buf, 1, MPI_INT, rank + 1, 19, MPI_COMM_WORLD);
        MPI_Recv(&buf, 1, MPI_INT, rank + 1, 19, MPI_COMM_WORLD, MPI_STATUS_IGNORE); // expected-warning{{Call is not reachable. Schema leads to a deadlock.}}
    }
    else if (rank == 1) {
        MPI_Send(&buf, 1, MPI_INT, rank - 1, 19, MPI_COMM_WORLD);
        MPI_Recv(&buf, 1, MPI_INT, rank - 1, 19, MPI_COMM_WORLD, MPI_STATUS_IGNORE); // expected-warning{{Call is not reachable. Schema leads to a deadlock.}}
    }
}

void matchedWait1() {
    int rank = 0;
    double buf = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank >= 0) {
        MPI_Request sendReq1,  recvReq1;
        MPI_Isend(&buf, 1, MPI_DOUBLE, rank + 1, 21, MPI_COMM_WORLD, &sendReq1);
        MPI_Irecv(&buf, 1, MPI_DOUBLE, rank - 1, 21, MPI_COMM_WORLD, &recvReq1);
        MPI_Request r[2] = {sendReq1, recvReq1};

        MPI_Waitall(2, r, MPI_STATUSES_IGNORE);
    }
} // no error

void matchedWait2() {
    int rank = 0;
    double buf = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank >= 0) {
        MPI_Request sendReq1,  recvReq1;
        MPI_Isend(&buf, 1, MPI_DOUBLE, rank + 1, 22, MPI_COMM_WORLD, &sendReq1);
        MPI_Irecv(&buf, 1, MPI_DOUBLE, rank - 1, 22, MPI_COMM_WORLD, &recvReq1);
        MPI_Request r[2] = {sendReq1, recvReq1};
        MPI_Wait(&sendReq1, MPI_STATUS_IGNORE);
        MPI_Wait(&recvReq1, MPI_STATUS_IGNORE);
    }
} // no error

void matchedWait3() {
    int rank = 0;
    double buf = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank >= 0) {
        MPI_Request sendReq1,  recvReq1;
        MPI_Isend(&buf, 1, MPI_DOUBLE, rank + 1, 23, MPI_COMM_WORLD, &sendReq1);
        MPI_Irecv(&buf, 1, MPI_DOUBLE, rank - 1, 23, MPI_COMM_WORLD, &recvReq1);
        MPI_Request r[2] = {sendReq1, recvReq1};

        if (rank > 1000) {
            MPI_Waitall(2, r, MPI_STATUSES_IGNORE);
        } else {
            MPI_Wait(&sendReq1, MPI_STATUS_IGNORE);
            MPI_Wait(&recvReq1, MPI_STATUS_IGNORE);
        }
    }
} // no error

void unmatchedWait() {
    int rank = 0;
    double buf = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0) {
        MPI_Request req;
        MPI_Wait(&req, MPI_STATUS_IGNORE); // expected-warning{{Request req has no matching nonblocking call.}}
    }
}
