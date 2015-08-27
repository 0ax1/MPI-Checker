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
#include <sys/time.h>
#include <mpi.h>

#ifdef HYBRID
#include <omp.h>
#endif

#include "partdiff-par.h"

// NOTE: The structs moved into the header file and have additional parameters
//   for the matrix splitting and the MPI rank and number of processes.

/* ************************************************************************ */
/* Global variables                                                         */
/* ************************************************************************ */

/* time measurement variables */
struct timeval start_time; /* time when program started                      */
struct timeval comp_time;  /* time when calculation completed                */

int rank;
int num;

/* ************************************************************************ */
/* initVariables: Initializes some global variables                         */
/* ************************************************************************ */
static void initVariables(struct calculation_arguments* arguments,
                          struct calculation_results* results,
                          struct options const* options) {
    uint64_t rest;
    uint64_t offset;
    uint64_t per;

    arguments->N = (options->interlines * 8) + 9 - 1;
    arguments->num_matrices = (options->method == METH_JACOBI) ? 2 : 1;
    arguments->h = 1.0 / arguments->N;

    // Split up the data. If rest is not zero, all processes which rank is
    // smaller than rest
    // get one additional line.
    rest = (arguments->N - 1) % num;
    per = (arguments->N - 1) / num;

    arguments->count = per;

    if (rank < rest) {
        arguments->count++;
        offset = rank;
    } else {
        offset = rest;
    }

    arguments->start = rank * per + offset + 1;
    arguments->end = arguments->start + arguments->count;

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
    uint64_t i, j;

    uint64_t const N = arguments->N;
    uint64_t const count = arguments->count;

    // Each process holds a matrix of the size (count + 2) x (N + 1).
    // The first and the last line are halo lines or the absolutely first or
    // absolutely last line
    // of the matrix.
    arguments->M = allocateMemory(arguments->num_matrices * (N + 1) *
                                  (count + 2) * sizeof(double));
    arguments->Matrix =
        allocateMemory(arguments->num_matrices * sizeof(double**));

    for (i = 0; i < arguments->num_matrices; i++) {
        arguments->Matrix[i] = allocateMemory((count + 2) * sizeof(double*));

        for (j = 0; j <= count + 1; j++) {
            arguments->Matrix[i][j] =
                arguments->M + (i * (count + 2) * (N + 1)) + (j * (N + 1));
        }
    }
}

/* ************************************************************************ */
/* initMatrices: Initialize matrix/matrices and some global variables       */
/* ************************************************************************ */
static void initMatrices(struct calculation_arguments* arguments,
                         struct options const* options) {
    uint64_t g, i, j, k; /*  local variables for loops   */

    uint64_t const N = arguments->N;
    uint64_t const count = arguments->count;
    double const h = arguments->h;
    double*** Matrix = arguments->Matrix;

    /* initialize matrix/matrices with zeros */
    for (g = 0; g < arguments->num_matrices; g++) {
        for (i = 0; i <= count + 1; i++) {
            for (j = 0; j <= N; j++) {
                Matrix[g][i][j] = 0.0;
            }
        }
    }

    /* initialize borders, depending on function (function 2: nothing to do) */

    // Changed for MPI:
    //  - k is the absolute index in the global matrix.
    if (options->inf_func == FUNC_F0) {
        for (g = 0; g < arguments->num_matrices; g++) {
            for (i = 0; i <= count + 1; i++) {
                k = i - 1 + arguments->start;
                Matrix[g][i][0] = 1.0 - (h * k);
                Matrix[g][i][N] = h * k;
            }

            if (rank == 0) {
                for (i = 0; i <= N; i++) {
                    Matrix[g][0][i] = 1.0 - (h * i);
                }
                Matrix[g][0][N] = 0.0;
            }
            if (arguments->end == N) {
                for (i = 0; i <= N; i++) {
                    Matrix[g][count + 1][i] = h * i;
                }
                Matrix[g][count + 1][0] = 0.0;
            }
        }
    }
}

#define PDE_CONTROL 1
#define PDE_RESIDUUM 2
#define PDE_LINE 4

#define PDE_CONTROL_FINISH 0
#define PDE_CONTROL_CONTINUE 1

// This is the Gauss-Seidel calculation method.

// General remark:
//  - MPI functions are only called, if there are at least two processes.
//    (Look in the conditions for the mpi stuff to see why this is true.)

static void calculate_gauss_seidel(
    struct calculation_arguments const* arguments,
    struct calculation_results* results, struct options const* options) {
    int i, j;           /* local variables for loops  */
    double star;        /* four times center value minus 4 neigh.b values */
    double residuum;    /* residuum of current iteration                  */
    double maxresiduum; /* maximum residuum value of a slave in iteration */

    int const N = arguments->N;
    double const h = arguments->h;
    uint64_t const count = arguments->count;
    int64_t const start = arguments->start;
    int64_t const end = arguments->end;

    double** Matrix = arguments->Matrix[0];

    double pih = 0.0;
    double fpisin = 0.0;

    int term_iteration = options->term_iteration;

    uint8_t first_iteration = 1;

    int finish = PDE_CONTROL_CONTINUE;
    int finish_sent = 0;

    if (options->inf_func == FUNC_FPISIN) {
        pih = PI * h;
        fpisin = 0.25 * TWO_PI_SQUARE * h * h;
    }

    while (term_iteration > 0) {
        MPI_Request send_top_halo;

        maxresiduum = 0;

        // Receve notice about the ending. rank 0 receives from the last
        // processes,
        // which controls the ending, the others from the process above them.
        // So, in every iteration, we send a notice, whether to break after
        // the next iteration.
        if (results->stat_iteration >= num - 1 &&
            options->termination == TERM_PREC) {
            if (rank == 0 && num > 1) {
                MPI_Recv(&finish, 1, MPI_INT, num - 1, PDE_CONTROL,
                         MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }

            if (rank > 0) {
                MPI_Recv(&finish, 1, MPI_INT, rank - 1, PDE_CONTROL,
                         MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
        }

        // Receive maxresiduum and halo line.
        if (rank > 0) {
            if (term_iteration == 1 || options->termination == TERM_PREC) {
                MPI_Recv(&maxresiduum, 1, MPI_DOUBLE, rank - 1, PDE_RESIDUUM,
                         MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }

            MPI_Recv(Matrix[0], N + 1, MPI_DOUBLE, rank - 1, PDE_LINE,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

        // We iterate only over the own rows.
        for (i = start; i < end; i++) {
            int mi = i - start + 1;  // i-th row in the matrix.
            double fpisin_i = 0.0;

            if (options->inf_func == FUNC_FPISIN) {
                fpisin_i = fpisin * sin(pih * (double)i);
            }

            // In the first iteration, we cannot receive anything.
            // Otherwise, receive the matrix when we need it.
            // This is no problem, since the sending of that line is
            // non-blocking.
            if (first_iteration == 0 && i == end - 1 && rank < num - 1) {
                MPI_Recv(Matrix[count + 1], N + 1, MPI_DOUBLE, rank + 1,
                         PDE_LINE, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }

            /* over all columns */
            for (j = 1; j < N; j++) {
                star = 0.25 * (Matrix[mi - 1][j] + Matrix[mi][j - 1] +
                               Matrix[mi][j + 1] + Matrix[mi + 1][j]);

                if (options->inf_func == FUNC_FPISIN) {
                    star += fpisin_i * sin(pih * (double)j);
                }

                if (options->termination == TERM_PREC || term_iteration == 1) {
                    residuum = Matrix[mi][j] - star;
                    residuum = (residuum < 0) ? -residuum : residuum;
                    maxresiduum =
                        (residuum < maxresiduum) ? maxresiduum : residuum;
                }

                Matrix[mi][j] = star;
            }

            // Send first computed line to the process that waits for it.
            // MPI_Wait for this is right below.
            if (i == start && rank > 0) {
                MPI_Isend(Matrix[1], N + 1, MPI_DOUBLE, (rank - 1), PDE_LINE,
                          MPI_COMM_WORLD, &send_top_halo);
            }
        }

        // If we never sent anything (i.e. if we have no lines),
        // we need to send it now, as the other process is waiting.
        if (start == end && rank > 0) {
            MPI_Isend(Matrix[1], N + 1, MPI_DOUBLE, (rank - 1), PDE_LINE,
                      MPI_COMM_WORLD, &send_top_halo);
        }

        // If we haven't received the bottom halo line (i.e. if we have no
        // lines),
        // we need to do this now to avoid deadlocks.
        if (first_iteration == 0 && start == end && rank < num - 1) {
            MPI_Recv(Matrix[count + 1], N + 1, MPI_DOUBLE, rank + 1, PDE_LINE,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

        // Wait for sending the top halo line. This *could* be at the end of the
        // loop,
        // but that reduces the readability of the code.
        if (rank > 0) {
            MPI_Wait(&send_top_halo, MPI_STATUS_IGNORE);
        }

        // Send the finish-notice just once when it is reached.
        if (finish_sent == 0 && rank == num - 1 && rank != 0 &&
            options->termination == TERM_PREC) {
            if (maxresiduum < options->term_precision) {
                finish = PDE_CONTROL_FINISH;
                finish_sent = 1;
            } else {
                finish = PDE_CONTROL_CONTINUE;
            }

            MPI_Ssend(&finish, 1, MPI_INT, 0, PDE_CONTROL, MPI_COMM_WORLD);

            // THIS IS NECESSARY. "finish" will be true again, when every
            // process
            // computed the n+k-1 th iteration. (see at the beginning of the
            // loop.)
            finish = PDE_CONTROL_CONTINUE;
        }

        // Handle this special case, where no finish-communcation is necessary.
        if (num == 1 && options->termination == TERM_PREC &&
            maxresiduum < options->term_precision) {
            finish = PDE_CONTROL_FINISH;
        }

        results->stat_iteration++;

        // We don't set the maxresiduum here, since there is no global
        // maximal residuum ;)

        first_iteration = 0;

        // If we need to, send a notice whether we are finished to the
        // bottom neighbour.
        if (results->stat_iteration >= num &&
            options->termination == TERM_PREC && rank < num - 1) {
            MPI_Ssend(&finish, 1, MPI_INT, rank + 1, PDE_CONTROL,
                      MPI_COMM_WORLD);
        }

        // Send residuum and matrix.
        if (rank < num - 1) {
            if (term_iteration == 1 || options->termination == TERM_PREC) {
                MPI_Ssend(&maxresiduum, 1, MPI_DOUBLE, rank + 1, PDE_RESIDUUM,
                          MPI_COMM_WORLD);
            }

            MPI_Ssend(Matrix[count], N + 1, MPI_DOUBLE, rank + 1, PDE_LINE,
                      MPI_COMM_WORLD);
        }

        /* check for stopping calculation, depending on termination method */
        if (options->termination == TERM_PREC && finish == PDE_CONTROL_FINISH) {
            term_iteration = 0;
        } else if (options->termination == TERM_ITER) {
            term_iteration--;
        }
    }

    // Receive the top halo lines to avoid a deadlock.
    if (rank < num - 1) {
        MPI_Recv(Matrix[count + 1], N + 1, MPI_DOUBLE, rank + 1, PDE_LINE,
                 MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    // DON'T PANIC.
    // This broadcast one of the the easiest (working) ways to send the residuum
    // to rank 0.
    // (send and recieve did not work :( ...)
    if (num > 1) {
        MPI_Bcast(&maxresiduum, 1, MPI_DOUBLE, num - 1, MPI_COMM_WORLD);
    }

    results->stat_precision = maxresiduum;
    results->m = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CALCULATION USING JACOBI'S METHOD
//
static void calculate_jacobi(struct calculation_arguments const* arguments,
                             struct calculation_results* results,
                             struct options const* options) {
    int i, j;           /* local variables for loops  */
    int m1, m2;         /* used as indices for old and new matrices       */
    double star;        /* four times center value minus 4 neigh.b values */
    double residuum;    /* residuum of current iteration                  */
    double maxresiduum; /* maximum residuum value of a slave in iteration */

    MPI_Status stat;

    int const N = arguments->N;
    double const h = arguments->h;
    uint64_t const count = arguments->count;
    int64_t const start = arguments->start;
    int64_t const end = arguments->end;

    double pih = 0.0;
    double fpisin = 0.0;

    uint8_t first_iteration = 1;

    int term_iteration = options->term_iteration;

    /* initialize m1 and m2 depending on algorithm */
    m1 = 0;
    m2 = 1;

    if (options->inf_func == FUNC_FPISIN) {
        pih = PI * h;
        fpisin = 0.25 * TWO_PI_SQUARE * h * h;
    }

    while (term_iteration > 0) {
        double** Matrix_Out = arguments->Matrix[m1];
        double** Matrix_In = arguments->Matrix[m2];

        MPI_Request send1, send2, recv1, recv2;

        // In the first iteration, wo do not communicate, we have everything.
        if (first_iteration == 0) {
            // Communicate. 0 sends to 1, and so on.
            // Then, the n-1 sends to n-2, and so on.
            // This may be optimized, but it would make things much more complex
            // and the overhead is low.
            if (rank > 0) {
                MPI_Isend(Matrix_In[1], N + 1, MPI_DOUBLE, (rank - 1), 3,
                          MPI_COMM_WORLD, &send1);
                MPI_Irecv(Matrix_In[0], N + 1, MPI_DOUBLE, (rank - 1), 2,
                          MPI_COMM_WORLD, &recv1);
            }

            // This is necessary, as the content we send might be overwritten
            // below.
            if (count <= 1 && rank > 0) {
                MPI_Wait(&send1, &stat);
                MPI_Wait(&recv1, &stat);
            }

            if (rank < num - 1) {
                MPI_Isend(Matrix_In[count], N + 1, MPI_DOUBLE, (rank + 1), 2,
                          MPI_COMM_WORLD, &send2);
                MPI_Irecv(Matrix_In[count + 1], N + 1, MPI_DOUBLE, (rank + 1),
                          3, MPI_COMM_WORLD, &recv2);
            }

            if (count > 1 && rank > 0) {
                MPI_Wait(&send1, &stat);
                MPI_Wait(&recv1, &stat);
            }
            if (rank < num - 1) {
                MPI_Wait(&send2, &stat);
                MPI_Wait(&recv2, &stat);
            }
        }

        first_iteration = 0;

        maxresiduum = 0;

/* over the own rows */
// Multi-Threading using OpenMP.
#ifdef HYBRID
#pragma omp parallel for private(j, star, residuum) reduction( \
    max : maxresiduum) num_threads(options->number)
#endif
        for (i = start; i < end; i++) {
            int mi = i - start + 1;  // i-th row in the matrix.
            double fpisin_i = 0.0;

            if (options->inf_func == FUNC_FPISIN) {
                fpisin_i = fpisin * sin(pih * (double)i);
            }

            /* over all columns */
            for (j = 1; j < N; j++) {
                star = 0.25 * (Matrix_In[mi - 1][j] + Matrix_In[mi][j - 1] +
                               Matrix_In[mi][j + 1] + Matrix_In[mi + 1][j]);

                if (options->inf_func == FUNC_FPISIN) {
                    star += fpisin_i * sin(pih * (double)j);
                }

                if (options->termination == TERM_PREC || term_iteration == 1) {
                    residuum = Matrix_In[mi][j] - star;
                    residuum = (residuum < 0) ? -residuum : residuum;
                    maxresiduum =
                        (residuum < maxresiduum) ? maxresiduum : residuum;
                }

                Matrix_Out[mi][j] = star;
            }
        }

        results->stat_iteration++;
        // That's not necessary here, see below.
        // results->stat_precision = maxresiduum;

        /* exchange m1 and m2 */
        i = m1;
        m1 = m2;
        m2 = i;

        /* check for stopping calculation, depending on termination method */
        if (options->termination == TERM_PREC || term_iteration == 1) {
            // Collect maximal maxresiduum from all processes to all processes.
            // Then, every process can decide whether to continue or to break.
            MPI_Allreduce(&maxresiduum, &maxresiduum, 1, MPI_DOUBLE, MPI_MAX,
                          MPI_COMM_WORLD);
            results->stat_precision = maxresiduum;
        }
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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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
    printf("Prozesse:           %d\n", (int)num);
    printf("Threads/Prozess:    %d\n", (int)options->number);
    // Changed: adjust memory usage stats per process
    printf("Speicherbedarf:     %f MiB\n",
           (arguments->count + 2) * (N + 1) * sizeof(double) *
               arguments->num_matrices / 1024.0 / 1024.0);
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

/* ************************************************************************ */
/*  main                                                                    */
/* ************************************************************************ */
int main(int argc, char** argv) {
    struct options options;
    struct calculation_arguments arguments;
    struct calculation_results results;
    int rc;

    // Init MPI and abort on failure
    rc = MPI_Init(&argc, &argv);

    if (rc != MPI_SUCCESS) {
        printf("MPI Error\n");
        MPI_Abort(MPI_COMM_WORLD, rc);
        return EXIT_FAILURE;
    }

    // Get information about the environment.
    MPI_Comm_size(MPI_COMM_WORLD, &num);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    /* get parameters */
    // NOTE: The function is MODIFIED AND DOES NOT SUPPORT STDIN ANY LONGER!
    // This now takes the rank as parameter to display the header only once.
    AskParams(&options, argc, argv, rank); /* ************************* */

    initVariables(&arguments, &results,
                  &options); /* ******************************************* */

    allocateMatrices(
        &arguments); /*  get and initialize variables and matrices  */
    initMatrices(&arguments,
                 &options); /* ******************************************* */

    gettimeofday(&start_time, NULL); /*  start timer         */
    if (options.method == METH_JACOBI) {
        calculate_jacobi(&arguments, &results, &options);
    } else {
        calculate_gauss_seidel(&arguments, &results, &options);
    }
    gettimeofday(&comp_time, NULL); /*  stop timer          */

    if (rank == 0) {
        // Rank 0 displays the stats. The computation time is not really
        // precise.
        displayStatistics(&arguments, &results, &options);
    }
    DisplayMatrix(&arguments, &results, &options, rank, num, arguments.start,
                  arguments.end - 1);

    freeMatrices(&arguments); /*  free memory     */

    MPI_Finalize();

    return EXIT_SUCCESS;
}
