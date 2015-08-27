/****************************************************************************/
/****************************************************************************/
/**                                                                        **/
/**                TU Muenchen - Institut fuer Informatik                  **/
/**                                                                        **/
/** Copyright: Prof. Dr. Thomas Ludwig                                     **/
/**            Andreas C. Schmidt                                          **/
/**            JK und andere  besseres Timing, FLOP Berechnung             **/
/**                                                                        **/
/** File:      partdiff-mpi.c                                              **/
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
#include <sys/time.h>
#include <mpi.h>

#include "partdiff.h"
#include "common-par.h"

struct calculation_arguments {
    uint64_t N;            /* number of spaces between lines (lines=N+1)     */
    uint64_t num_matrices; /* number of matrices                             */
    double h;              /* length of a space between two lines            */
    double*** Matrix;      /* index matrix used for addressing M             */
    double* M;             /* two matrices with real values                  */
    // MPI variables:
    uint64_t from;
    uint64_t to;
    uint64_t lines;
};

struct calculation_results {
    uint64_t m;
    uint64_t stat_iteration; /* number of current iteration */
    double stat_precision; /* actual precision of all slaves in iteration    */
};

int rank;
int nprocs;

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

// TODO : Comments
#define MIN(a, b) ((a) < (b) ? (a) : (b))
    int q = (arguments->N - 1) / nprocs, r = (arguments->N - 1) % nprocs;
    arguments->from = 1 + (rank * q) + MIN(rank, r);
    arguments->to = arguments->from - 1 + q + (rank < r ? 1 : 0);
    arguments->lines = arguments->to - arguments->from + 1;
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
static void* allocateMemory(size_t size) {
    void* p;

    if ((p = malloc(size)) == NULL) {
        printf("Speicherprobleme! (%" PRIu64 " Bytes)\n", size);
        /* exit program */
        exit(1);
    }

    return p;
}

/* ************************************************************************ */
/* allocateMatrices: allocates memory for matrices                          */
/* ************************************************************************ */
static void allocateMatrices(struct calculation_arguments* arguments) {
    // TODO: Comments
    uint64_t i, j;

    uint64_t lines = arguments->lines + 2;

    uint64_t const N = arguments->N;

    arguments->M = allocateMemory(arguments->num_matrices * lines * (N + 1) *
                                  sizeof(double));
    arguments->Matrix =
        allocateMemory(arguments->num_matrices * sizeof(double**));

    for (i = 0; i < arguments->num_matrices; i++) {
        arguments->Matrix[i] = allocateMemory(lines * sizeof(double*));

        for (j = 0; j < lines; j++) {
            arguments->Matrix[i][j] =
                arguments->M + (i * lines * (N + 1)) + (j * (N + 1));
        }
    }
}

/* ************************************************************************ */
/* initMatrices: Initialize matrix/matrices and some global variables       */
/* ************************************************************************ */
static void initMatrices(struct calculation_arguments* arguments,
                         struct options const* options) {
    uint64_t g, i, j; /*  local variables for loops   */

    uint64_t const N = arguments->N;
    uint64_t lines = arguments->lines;
    double const h = arguments->h;
    double*** Matrix = arguments->Matrix;

    /* initialize matrix/matrices with zeros */
    for (g = 0; g < arguments->num_matrices; g++) {
        // TODO: Comments
        for (i = 0; i <= lines + 1; i++) {
            for (j = 0; j <= N; j++) {
                Matrix[g][i][j] = 0.0;
            }
        }
    }

    /* initialize borders, depending on function (function 2: nothing to do) */
    if (options->inf_func == FUNC_F0) {
        for (g = 0; g < arguments->num_matrices; g++) {
            for (i = 0; i <= N; i++) {
                // arguments->from -1 --> arguments->to + 1
                if (arguments->from - 1 <= i && i <= arguments->to + 1) {
                    int pos = i - arguments->from + 1;
                    Matrix[g][pos][0] = 1.0 - (h * i);
                    Matrix[g][pos][N] = h * i;
                }
                if (rank == 0)
                    Matrix[g][0][i] = 1.0 - (h * i);
                else if (rank == nprocs - 1)
                    Matrix[g][arguments->lines + 1][i] = h * i;
            }

            if (rank == nprocs - 1) Matrix[g][arguments->lines + 1][0] = 0.0;
            if (rank == 0) Matrix[g][0][N] = 0.0;
        }
    }
}

static void calculate_jacobi(struct calculation_arguments const* arguments,
                             struct calculation_results* results,
                             struct options const* options) {
    // MPI!!
    int i, j;           /* local variables for loops  */
    double residuum;    /* residuum of current iteration                  */
    double maxresiduum; /* maximum residuum value of a slave in iteration */

    int const N = arguments->N;
    int const lines = arguments->lines;
    double const h = arguments->h;

    double pih = 0.0;
    double fpisin = 0.0;

    int term_iteration = options->term_iteration;

    int m1 = 0, m2 = 1; /* used as indices for old and new matrices       */

    if (options->inf_func == FUNC_FPISIN) {
        pih = PI * h;
        fpisin = 0.25 * TWO_PI_SQUARE * h * h;
    }

    while (term_iteration > 0) {
        double** Matrix_Out = arguments->Matrix[m1];
        double** Matrix_In = arguments->Matrix[m2];

        maxresiduum = 0;

        /* over all rows */
        for (i = 1; i <= lines; i++) {
            double fpisin_i = 0.0;

            if (options->inf_func == FUNC_FPISIN) {
                fpisin_i =
                    fpisin * sin(pih * (double)(i + arguments->from - 1));
            }

            /* over all columns */
            for (j = 1; j < N; j++) {
                double star =
                    0.25 * (Matrix_In[i - 1][j] + Matrix_In[i][j - 1] +
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

        if (options->termination == TERM_PREC || term_iteration == 1)
            MPI_Allreduce(&maxresiduum, &results->stat_precision, 1, MPI_DOUBLE,
                          MPI_MAX, MPI_COMM_WORLD);

        results->stat_iteration++;

        /* exchange m1 and m2 */
        i = m1;
        m1 = m2;
        m2 = i;

        /* check for stopping calculation, depending on termination method */
        if (options->termination == TERM_PREC) {
            if (results->stat_precision < options->term_precision) {
                term_iteration = 0;
            }
        } else if (options->termination == TERM_ITER) {
            term_iteration--;
        }

        // Communication!
        if (term_iteration) {
            if (rank != 0)
                MPI_Sendrecv(Matrix_Out[1], N + 1, MPI_DOUBLE, rank - 1, 0,
                             Matrix_Out[0], N + 1, MPI_DOUBLE, rank - 1, 0,
                             MPI_COMM_WORLD, NULL);
            if (rank != nprocs - 1)
                MPI_Sendrecv(Matrix_Out[lines], N + 1, MPI_DOUBLE, rank + 1, 0,
                             Matrix_Out[lines + 1], N + 1, MPI_DOUBLE, rank + 1,
                             0, MPI_COMM_WORLD, NULL);
        }
    }

    results->m = m2;
}

static void calculate_gauss(struct calculation_arguments const* arguments,
                            struct calculation_results* results,
                            struct options const* options) {
    int i, j;            /* local variables for loops  */
    double star;         /* four times center value minus 4 neigh.b values */
    double residuum;     /* residuum of current iteration                  */
    double maxresiduum;  /* maximum residuum value of a slave in iteration */
    int stopSignal = 0;  // Only necessary for PRECISION termination

    int const N = arguments->N;
    int const lines = arguments->lines;
    double const h = arguments->h;

    double pih = 0.0;
    double fpisin = 0.0;

    int term_iteration = options->term_iteration;

    // ### Code changed: just one Matrix necessary
    double** Matrix = arguments->Matrix[0];
    MPI_Request request_first_line, request_stop = NULL;

    if (options->inf_func == FUNC_FPISIN) {
        pih = PI * h;
        fpisin = 0.25 * TWO_PI_SQUARE * h * h;
    }

    while (term_iteration > 0) {
        maxresiduum = 0;

        // ### Code changed: Receive predecessor's last line, predecessor's
        // maxresiduum and stopSignal (first & last value of predecessors's
        // line are not needed, maxresiduum & stopSignal are placed there.)
        if (rank != 0) {
            MPI_Recv(Matrix[0], N + 1, MPI_DOUBLE, rank - 1, 0, MPI_COMM_WORLD,
                     NULL);

            maxresiduum = Matrix[0][0];
            stopSignal = Matrix[0][N];
        }

        /* over all rows */
        for (i = 1; i <= lines; i++) {
            double fpisin_i = 0.0;

            if (options->inf_func == FUNC_FPISIN) {
                fpisin_i =
                    fpisin * sin(pih * (double)(i + arguments->from - 1));
            }

            if ((rank != nprocs - 1) && (i == lines) &&
                (results->stat_iteration != 0)) {
                MPI_Recv(Matrix[lines + 1], N + 1, MPI_DOUBLE, rank + 1, 0,
                         MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }

            /* over all columns */
            for (j = 1; j < N; j++) {
                star = 0.25 * (Matrix[i - 1][j] + Matrix[i][j - 1] +
                               Matrix[i][j + 1] + Matrix[i + 1][j]);

                if (options->inf_func == FUNC_FPISIN) {
                    star += fpisin_i * sin(pih * (double)j);
                }

                if (options->termination == TERM_PREC || term_iteration == 1) {
                    residuum = Matrix[i][j] - star;
                    residuum = (residuum < 0) ? -residuum : residuum;
                    maxresiduum =
                        (residuum < maxresiduum) ? maxresiduum : residuum;
                }

                Matrix[i][j] = star;
            }
            if ((rank != 0) && (i == 1)) {
                if (results->stat_iteration != 0)
                    MPI_Wait(&request_first_line, MPI_STATUS_IGNORE);
                MPI_Isend(Matrix[1], N + 1, MPI_DOUBLE, rank - 1, 0,
                          MPI_COMM_WORLD, &request_first_line);
            }
        }

        // ### Code changed: stopSignal available? (MPI_Tag: 8)
        if ((options->termination == TERM_PREC) && (rank == 0)) {
            int flag = 0;
            MPI_Status status;

            MPI_Iprobe(nprocs - 1, 8, MPI_COMM_WORLD, &flag, &status);
            if (flag) {
                int t;
                MPI_Recv(&t, 1, MPI_INT, nprocs - 1, 8, MPI_COMM_WORLD, NULL);
                stopSignal = 1;
            }
        }

        // ### Code changed: Send last line, maxresiduum and Stop-Signal to
        // successor, put maxresiduum & stopSignal at first & last place.
        if (rank != nprocs - 1) {
            double temp[2] = {Matrix[lines][0], Matrix[lines][N]};
            Matrix[lines][0] = maxresiduum;
            Matrix[lines][N] = stopSignal;
            MPI_Send(Matrix[lines], N + 1, MPI_DOUBLE, rank + 1, 0,
                     MPI_COMM_WORLD);
            Matrix[lines][0] = temp[0];
            Matrix[lines][N] = temp[1];
        }

        results->stat_iteration++;
        results->stat_precision = maxresiduum;

        /* check for stopping calculation, depending on termination method */
        if (options->termination == TERM_PREC) {
            // ### Code changed: Notify first process that stop-criteria is
            // achieved
            if ((rank == nprocs - 1) &&
                (maxresiduum < options->term_precision)) {
                int t = 0;  // empty message...
                // Send only one time...
                if (request_stop == NULL)
                    MPI_Isend(&t, 1, MPI_INT, 0, 8, MPI_COMM_WORLD,
                              &request_stop);
                // It is not necessary to know/test when the signal is received
            }
            if (stopSignal > 0) {
                term_iteration = 0;
            }
        } else if (options->termination == TERM_ITER) {
            term_iteration--;
        }
    }

    // ### Code changed: Send maxresiduum to first process
    if (rank == 0 && nprocs > 1)
        MPI_Recv(&results->stat_precision, 1, MPI_DOUBLE, nprocs - 1, 13,
                 MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    if (rank == nprocs - 1 && nprocs > 1)
        MPI_Send(&results->stat_precision, 1, MPI_DOUBLE, 0, 13,
                 MPI_COMM_WORLD);
    results->m = 0;
}

/* ************************************************************************ */
/* calculate: solves the equation                                           */
/* ************************************************************************ */
static void calculate(struct calculation_arguments const* arguments,
                      struct calculation_results* results,
                      struct options const* options) {
    if (options->method == METH_JACOBI) {
        calculate_jacobi(arguments, results, options);
    } else {
        calculate_gauss(arguments, results, options);
    }
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

/****************************************************************************/
/** Beschreibung der Funktion DisplayMatrix:                               **/
/**                                                                        **/
/** Die Funktion DisplayMatrix gibt eine Matrix                            **/
/** in einer "ubersichtlichen Art und Weise auf die Standardausgabe aus.   **/
/**                                                                        **/
/** Die "Ubersichtlichkeit wird erreicht, indem nur ein Teil der Matrix    **/
/** ausgegeben wird. Aus der Matrix werden die Randzeilen/-spalten sowie   **/
/** sieben Zwischenzeilen ausgegeben.                                      **/
/****************************************************************************/
extern void DisplayMatrix_MPI(char* s, double* v, int interlines, int rank,
                              int size, int from, int to);

static void DisplayMatrix(struct calculation_arguments* arguments,
                          struct calculation_results* results,
                          struct options* options) {
    // MPI
    DisplayMatrix_MPI("Matrix:", arguments->Matrix[results->m][0],
                      options->interlines, rank, nprocs, arguments->from,
                      arguments->to);
}

/* ************************************************************************ */
/*  main                                                                    */
/* ************************************************************************ */
int main(int argc, char** argv) {
    struct options options;
    struct calculation_arguments arguments;
    struct calculation_results results;

    // MPI
    MPI_doSetup(&argc, &argv, &nprocs, &rank);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    /* get parameters */
    if (rank == 0)
        AskParams(&options, argc, argv); /* ************************* */

    MPI_Bcast(&options, sizeof(struct options), MPI_BYTE, 0, MPI_COMM_WORLD);

    initVariables(&arguments, &results, &options);

    allocateMatrices(
        &arguments); /* get and initialize variables and matrices */
    initMatrices(&arguments, &options); /* *************************** */

    gettimeofday(&start_time, NULL);           /*  start timer         */
    calculate(&arguments, &results, &options); /*  solve the equation  */
    gettimeofday(&comp_time, NULL);            /*  stop timer          */

    if (rank == 0) displayStatistics(&arguments, &results, &options);
    DisplayMatrix(&arguments, &results, &options);

    freeMatrices(&arguments); /*  free memory     */

    return 0;
}
