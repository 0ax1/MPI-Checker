#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <mpi.h>

static void errhandler(__attribute__((unused)) MPI_Comm* comm, int* error) {
    char errstring[MPI_MAX_ERROR_STRING];
    int number = MPI_MAX_ERROR_STRING;
    MPI_Error_string(*error, errstring, &number);
    fprintf(stderr, "Error: %s\n", errstring);
    exit(-1);
}

// Diese Funktion richtet MPI für den Prozess ein.
// - Initalisierung der MPI Runtime
// - Anpassung der Command line
// - Fehlerbehandlung
// - MPI_Finalize als Callback beim Beenden
void MPI_doSetup(int* argc, char*** argv, int* size, int* rank) {
    int err;
    if ((err = MPI_Init(argc, argv))) errhandler(NULL, &err);

    // Setting up error handler as early as possible
    /* MPI_Errhandler errh; */
    /* MPI_Comm_create_errhandler((MPI_Comm_errhandler_fn*)&errhandler, &errh);
     */
    /* MPI_Comm_set_errhandler(MPI_COMM_WORLD, errh); */

    MPI_Comm_size(MPI_COMM_WORLD, size);
    MPI_Comm_rank(MPI_COMM_WORLD, rank);

    atexit((void (*)(void)) & MPI_Finalize);
}
