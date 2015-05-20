/****************************************************************************/
/****************************************************************************/
/**                                                                        **/
/**                TU Muenchen - Institut fuer Informatik                  **/
/**                                                                        **/
/** Copyright: Prof. Dr. Thomas Ludwig                                     **/
/**            Andreas C. Schmidt                                          **/
/**                                                                        **/
/** File:      partdiff-seq.c                                              **/
/**                                                                        **/
/** Purpose:   Partial differential equation solver for Gauss-Seidel and   **/
/**            Jacobi method.                                              **/
/**                                                                        **/
/****************************************************************************/
/****************************************************************************/

/* ************************************************************************ */
/* Include standard header file.                                            */
/* ************************************************************************ */
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <math.h>
#include <mpi.h>
#include <sys/time.h>

int rank, processCount;
MPI_Status status;
MPI_Request request;

#include "partdiff-par.h"

struct calculation_arguments {
    uint64_t N;            /* number of spaces between lines (lines=N+1)     */
    uint64_t num_matrices; /* number of matrices                             */
    double h;              /* length of a space between two lines            */
    double*** Matrix;      /* index matrix used for addressing M             */
    double* M;             /* two matrices with real values                  */
};

struct calculation_results {
    uint64_t m;
    uint64_t stat_iteration; /* number of current iteration */
    double stat_precision; /* actual precision of all slaves in iteration    */
};

/* ************************************************************************ */
/* Global variables                                                         */
/* ************************************************************************ */

/* time measurement variables */
struct timeval start_time; /* time when program started                      */
struct timeval comp_time;  /* time when calculation completed                */

/* ************************************************************************ */
/* initVariables: Initializes some global variables                         */
/* ************************************************************************ */
static void initVariables(struct calculation_arguments* arguments,
                          struct calculation_results* results,
                          struct options const* options) {
    arguments->N = (options->interlines * 8) + 9 - 1;
    arguments->num_matrices = (options->method == METH_JACOBI) ? 2 : 1;
    arguments->h = 1.0 / arguments->N;

    results->m = 0;
    results->stat_iteration = 0;
    results->stat_precision = 0;
}

/* ************************************************************************ */
/* freeMatrices: frees memory for matrices                                  */
/* ************************************************************************ */
static void freeMatrices(struct calculation_arguments* arguments) {
    uint64_t i;

    for (i = 0; i < arguments->num_matrices; i++) {
        free(arguments->Matrix[i]);
    }

    free(arguments->Matrix);
    free(arguments->M);
}

/* ************************************************************************ */
/* allocateMemory ()                                                        */
/* allocates memory and quits if there was a memory allocation problem      */
/* ************************************************************************ */
static void* allocateMemory(unsigned count, size_t size) {
    void* p;

    if ((p = calloc(count, size)) == NULL) {
        printf("Speicherprobleme!\n");
        /* exit program */
        exit(1);
    }

    return p;
}

/* ************************************************************************ */
/* allocateMatrices: allocates memory for matrices                          */
/* ************************************************************************ */
static void allocateMatrices(struct calculation_arguments* arguments,
                             int localRowCount) {
    uint64_t const N = arguments->N;

    arguments->M = allocateMemory(
        arguments->num_matrices * localRowCount * (N + 1), sizeof(double));
    arguments->Matrix =
        allocateMemory(arguments->num_matrices, sizeof(double**));

    for (unsigned i = 0; i < arguments->num_matrices; i++) {
        arguments->Matrix[i] = allocateMemory(localRowCount, sizeof(double*));

        for (int j = 0; j < localRowCount; ++j) {
            arguments->Matrix[i][j] =
                arguments->M + (i * localRowCount * (N + 1)) + (j * (N + 1));
        }
    }
}

/* ************************************************************************ */
/* initMatrices: Initialize matrix/matrices and some global variables       */
/* ************************************************************************ */
// validated
static void initMatrices(struct calculation_arguments* arguments,
                         struct options const* options, int localRowCount,
                         int startRowIdx) {
    uint64_t g; /*  local variables for loops   */

    uint64_t const N = arguments->N;
    double const h = arguments->h;
    double*** Matrix = arguments->Matrix;

    /* initialize borders, depending on function (function 2: nothing to do) */
    if (options->inf_func == FUNC_F0) {
        for (g = 0; g < arguments->num_matrices; ++g) {
            // borders (left/right), every process
            for (int i = 0; i < localRowCount; ++i) {
                Matrix[g][i][0] = 1.0 - (h * (i + startRowIdx));
                Matrix[g][i][N] = h * (i + startRowIdx);
            }

            // top first process
            if (rank == 0) {
                for (uint64_t i = 0; i <= N; ++i) {
                    Matrix[g][0][i] = 1.0 - (h * i);
                }
                Matrix[g][0][N] = 0.0;
            }
            // bottom last process
            else if (rank == processCount - 1) {
                for (uint64_t i = 0; i <= N; ++i) {
                    Matrix[g][localRowCount - 1][i] = h * (i + startRowIdx);
                }
                Matrix[g][localRowCount - 1][0] = 0.0;
            }
        }
    }
}

static void calculateSeq(struct calculation_arguments const* arguments,
                         struct calculation_results* results,
                         struct options const* options) {
    int i, j;           /* local variables for loops  */
    int m1, m2;         /* used as indices for old and new matrices       */
    double star;        /* four times center value minus 4 neigh.b values */
    double residuum;    /* residuum of current iteration                  */
    double maxresiduum; /* maximum residuum value of a slave in iteration */

    int const N = arguments->N;
    double const h = arguments->h;

    double pih = 0.0;
    double fpisin = 0.0;

    int term_iteration = options->term_iteration;

    /* initialize m1 and m2 depending on algorithm */
    if (options->method == METH_JACOBI) {
        m1 = 0;
        m2 = 1;
    } else {
        m1 = 0;
        m2 = 0;
    }

    if (options->inf_func == FUNC_FPISIN) {
        pih = PI * h;
        fpisin = 0.25 * TWO_PI_SQUARE * h * h;
    }

    while (term_iteration > 0) {
        double** Matrix_Out = arguments->Matrix[m1];
        double** Matrix_In = arguments->Matrix[m2];

        maxresiduum = 0;

        /* over all rows */
        for (i = 1; i < N; i++) {
            double fpisin_i = 0.0;

            if (options->inf_func == FUNC_FPISIN) {
                fpisin_i = fpisin * sin(pih * (double)i);
            }

            /* over all columns */
            for (j = 1; j < N; j++) {
                star = 0.25 * (Matrix_In[i - 1][j] + Matrix_In[i][j - 1] +
                               Matrix_In[i][j + 1] + Matrix_In[i + 1][j]);

                if (options->inf_func == FUNC_FPISIN) {
                    star += fpisin_i * sin(pih * (double)j);
                }

                if (options->termination == TERM_PREC || term_iteration == 1) {
                    residuum = Matrix_In[i][j] - star;
                    residuum = (residuum < 0) ? -residuum : residuum;
                    maxresiduum =
                        (residuum < maxresiduum) ? maxresiduum : residuum;
                }

                Matrix_Out[i][j] = star;
            }
        }

        results->stat_iteration++;
        results->stat_precision = maxresiduum;

        /* exchange m1 and m2 */
        i = m1;
        m1 = m2;
        m2 = i;

        /* check for stopping calculation, depending on termination method */
        if (options->termination == TERM_PREC) {
            if (maxresiduum < options->term_precision) {
                term_iteration = 0;
            }
        } else if (options->termination == TERM_ITER) {
            term_iteration--;
        }
    }

    results->m = m2;
}

static void calculateJacobi(struct calculation_arguments const* arguments,
                            struct calculation_results* results,
                            struct options const* options, int localRowCount,
                            int startRowIdx) {
    int i, j;
    int m1, m2;
    double star;
    double residuum;
    double maxresiduum;

    int const N = arguments->N;
    double const h = arguments->h;

    double pih = 0.0;
    double fpisin = 0.0;

    int term_iteration = options->term_iteration;

    if (options->method == METH_JACOBI) {
        m1 = 0;
        m2 = 1;
    } else {
        m1 = 0;
        m2 = 0;
    }

    if (options->inf_func == FUNC_FPISIN) {
        pih = PI * h;
        fpisin = 0.25 * TWO_PI_SQUARE * h * h;
    }

    while (term_iteration > 0) {
        double** Matrix_Out = arguments->Matrix[m1];
        double** Matrix_In = arguments->Matrix[m2];

        maxresiduum = 0;

        for (i = 1; i < localRowCount - 1; ++i) {
            double fpisin_i = 0.0;

            if (options->inf_func == FUNC_FPISIN) {
                fpisin_i = fpisin * sin(pih * (double)(i + startRowIdx));
            }

            for (j = 1; j < N; ++j) {
                star = 0.25 * (Matrix_In[i - 1][j] + Matrix_In[i][j - 1] +
                               Matrix_In[i][j + 1] + Matrix_In[i + 1][j]);

                if (options->inf_func == FUNC_FPISIN) {
                    star += fpisin_i * sin(pih * (double)j);
                }

                if (options->termination == TERM_PREC || term_iteration == 1) {
                    residuum = Matrix_In[i][j] - star;
                    residuum = (residuum < 0) ? -residuum : residuum;
                    maxresiduum =
                        (residuum < maxresiduum) ? maxresiduum : residuum;
                }

                Matrix_Out[i][j] = star;
            }
        }

        MPI_Request sendReq1, sendReq2;
        MPI_Request recvReq1, recvReq2;

        if (processCount > 1) {
            // first rank
            if (rank == 0) {
                // send to next
                MPI_Isend(Matrix_Out[localRowCount - 2], (N + 1), MPI_DOUBLE,
                          rank + 1, 0, MPI_COMM_WORLD, &sendReq1);
                // recv from next
                MPI_Irecv(Matrix_Out[localRowCount - 1], (N + 1), MPI_DOUBLE,
                          rank - 1, 0, MPI_COMM_WORLD, &recvReq1);

                MPI_Wait(&sendReq1, MPI_STATUS_IGNORE);
                MPI_Wait(&recvReq1, MPI_STATUS_IGNORE);
            }
            // last rank
            else if (rank == processCount - 1) {
                // send to prev
                MPI_Isend(Matrix_Out[1], (N + 1), MPI_DOUBLE, rank - 1, 0,
                          MPI_COMM_WORLD, &sendReq1);

                // recv from prev
                MPI_Irecv(Matrix_Out[0], (N + 1), MPI_DOUBLE, rank - 1, 0,
                          MPI_COMM_WORLD, &recvReq1);

                MPI_Wait(&sendReq1, MPI_STATUS_IGNORE);
                MPI_Wait(&recvReq1, MPI_STATUS_IGNORE);
            }
            // in between
            else {
                // send to next
                MPI_Isend(Matrix_Out[localRowCount - 2], (N + 1), MPI_DOUBLE,
                          rank + 1, 0, MPI_COMM_WORLD, &sendReq1);

                // recv from next
                MPI_Irecv(Matrix_Out[localRowCount - 1], (N + 1), MPI_DOUBLE,
                          rank + 1, 0, MPI_COMM_WORLD, &recvReq1);

                // send to prev
                MPI_Isend(Matrix_Out[1], (N + 1), MPI_DOUBLE, rank - 1, 0,
                          MPI_COMM_WORLD, &sendReq2);

                // recv from prev
                MPI_Irecv(Matrix_Out[0], (N + 1), MPI_DOUBLE, rank - 1, 0,
                          MPI_COMM_WORLD, &recvReq2);

                MPI_Wait(&sendReq1, MPI_STATUS_IGNORE);
                MPI_Wait(&recvReq1, MPI_STATUS_IGNORE);
                MPI_Wait(&sendReq2, MPI_STATUS_IGNORE);
                MPI_Wait(&recvReq2, MPI_STATUS_IGNORE);
            }
        }

        results->stat_iteration++;

        // get global max residuum
        MPI_Allreduce(MPI_IN_PLACE, &maxresiduum, 1, MPI_DOUBLE, MPI_MAX,
                      MPI_COMM_WORLD);
        results->stat_precision = maxresiduum;

        i = m1;
        m1 = m2;
        m2 = i;

        if (options->termination == TERM_PREC) {
            if (maxresiduum < options->term_precision) {
                term_iteration = 0;
            }
        } else if (options->termination == TERM_ITER) {
            term_iteration--;
        }
    }

    results->m = m2;
}

/* ************************************************************************ */
/*  displayStatistics: displays some statistics about the calculation       */
/* ************************************************************************ */
static void displayStatistics(struct calculation_arguments const* arguments,
                              struct calculation_results const* results,
                              struct options const* options) {
    int N = arguments->N;
    double time = (comp_time.tv_sec - start_time.tv_sec) +
                  (comp_time.tv_usec - start_time.tv_usec) * 1e-6;

    printf("Berechnungszeit:    %f s \n", time);
    printf("Speicherbedarf:     %f MiB\n", (N + 1) * (N + 1) * sizeof(double) *
                                               arguments->num_matrices /
                                               1024.0 / 1024.0);
    printf("Berechnungsmethode: ");

    if (options->method == METH_GAUSS_SEIDEL) {
        printf("Gauss-Seidel");
    } else if (options->method == METH_JACOBI) {
        printf("Jacobi");
    }

    printf("\n");
    printf("Interlines:         %" PRIu64 "\n", options->interlines);
    printf("Stoerfunktion:      ");

    if (options->inf_func == FUNC_F0) {
        printf("f(x,y) = 0");
    } else if (options->inf_func == FUNC_FPISIN) {
        printf("f(x,y) = 2pi^2*sin(pi*x)sin(pi*y)");
    }

    printf("\n");
    printf("Terminierung:       ");

    if (options->termination == TERM_PREC) {
        printf("Hinreichende Genaugkeit");
    } else if (options->termination == TERM_ITER) {
        printf("Anzahl der Iterationen");
    }

    printf("\n");
    printf("Anzahl Iterationen: %" PRIu64 "\n", results->stat_iteration);
    printf("Norm des Fehlers:   %e\n", results->stat_precision);
    printf("\n");
}

static void DisplayMatrix(struct calculation_arguments* arguments,
                          struct calculation_results* results,
                          struct options* options, int rank, int size, int from,
                          int to) {
    int const elements = 8 * options->interlines + 9;

    int x, y;
    double** Matrix = arguments->Matrix[results->m];
    MPI_Status status;

    /* first line belongs to rank 0 */
    if (rank == 0) from--;

    /* last line belongs to rank size - 1 */
    if (rank + 1 == size) to++;

    if (rank == 0) printf("Matrix:\n");

    for (y = 0; y < 9; y++) {
        int line = y * (options->interlines + 1);

        if (rank == 0) {
            /* check whether this line belongs to rank 0 */
            if (line < from || line > to) {
                /* use the tag to receive the lines in the correct order
                 * the line is stored in Matrix[0], because we do not need it
                 * anymore */
                MPI_Recv(Matrix[0], elements, MPI_DOUBLE, MPI_ANY_SOURCE,
                         42 + y, MPI_COMM_WORLD, &status);
            }
        } else {
            if (line >= from && line <= to) {
                /* if the line belongs to this process, send it to rank 0
                 * (line - from + 1) is used to calculate the correct local
                 * address */
                MPI_Send(Matrix[line - from + 1], elements, MPI_DOUBLE, 0,
                         42 + y, MPI_COMM_WORLD);
            }
        }

        if (rank == 0) {
            for (x = 0; x < 9; x++) {
                int col = x * (options->interlines + 1);

                if (line >= from && line <= to) {
                    /* this line belongs to rank 0 */
                    printf("%7.4f", Matrix[line][col]);
                } else {
                    /* this line belongs to another rank and was received above
                     */
                    printf("%7.4f", Matrix[0][col]);
                }
            }

            printf("\n");
        }
    }

    fflush(stdout);
}

// validated
void calcRowSetup(int completeRowCount, int* localRowCount, int* startRow) {
    int startRow_ = 0;
    int localRowCount_ = 0;

    int remainingProcessCount = processCount;
    for (int i = 0; i < processCount; ++i) {
        localRowCount_ =
            ceil((float)completeRowCount / (float)(remainingProcessCount));

        startRow_ += localRowCount_;

        if (rank == i) break;
        completeRowCount -= localRowCount_;
        --remainingProcessCount;
    }

    startRow_ -= localRowCount_;

    if (rank > 0) {
        // process has predecessor
        ++localRowCount_;
        --startRow_;
    }

    ++localRowCount_;

    *startRow = startRow_;
    *localRowCount = localRowCount_;
}

/* ************************************************************************ */
/*  main                                                                    */
/* ************************************************************************ */
int main(int argc, char** argv) {
    struct options options;
    struct calculation_arguments arguments;
    struct calculation_results results;

    // start MPI–––––––––––––––––––––––––––––––––––––––––––––––––
    MPI_Init(NULL, NULL);  // Get number of processes
    MPI_Comm_size(MPI_COMM_WORLD, &processCount);

    // Get rank for this process
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    //––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

    // init–––––––––––––––––––––––––––––––––––––––––––––––––––––
    AskParams(&options, argc, argv, rank);
    initVariables(&arguments, &results, &options);

    int localRowCount;
    int startRowIdx;
    calcRowSetup(arguments.N, &localRowCount, &startRowIdx);

    allocateMatrices(&arguments, localRowCount);
    initMatrices(&arguments, &options, localRowCount, startRowIdx);
    //––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

    gettimeofday(&start_time, NULL); /*  start timer  */
    if (processCount == 1) {
        calculateSeq(&arguments, &results, &options);
    } else {
        if (options.method == METH_JACOBI) {
            calculateJacobi(&arguments, &results, &options, localRowCount,
                            startRowIdx);
        }
    }
    gettimeofday(&comp_time, NULL); /*  stop timer  */

    if (rank == 0) {
        displayStatistics(&arguments, &results, &options);
    }

    DisplayMatrix(&arguments, &results, &options, rank, processCount,
                  startRowIdx + 1, startRowIdx + localRowCount - 2);

    freeMatrices(&arguments); /*  free memory  */
    MPI_Finalize();
    return 0;
}
