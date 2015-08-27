#include <mpi.h>
#include <stdio.h>

void DisplayMatrix_MPI(char *s, double *v, int interlines, int rank, int size,
                       int from, int to) {
    int x, y;
    int lines = 8 * interlines + 9;
    MPI_Status status;

    /* first line belongs to rank 0 */
    if (rank == 0) from--;

    /* last line belongs to rank size-1 */
    if (rank + 1 == size) to++;

    if (rank == 0) printf("%s\n", s);

    for (y = 0; y < 9; y++) {
        int line = y * (interlines + 1);

        if (rank == 0) {
            /* check whether this line belongs to rank 0 */
            if (line < from || line > to) {
                /* use the tag to receive the lines in the correct order
                 * the line is stored in v, because we do not need it anymore */
                MPI_Recv(v, lines, MPI_DOUBLE, MPI_ANY_SOURCE, 42 + y,
                         MPI_COMM_WORLD, &status);
            }
        } else {
            if (line >= from && line <= to) {
                /* if the line belongs to this process, send it to rank 0
                 * (line - from + 1) is used to calculate the correct local
                 * address */
                MPI_Send(&v[(line - from + 1) * lines], lines, MPI_DOUBLE, 0,
                         42 + y, MPI_COMM_WORLD);
            }
        }

        for (x = 0; x < 9; x++) {
            if (rank == 0) {
                if (line >= from && line <= to) {
                    /* this line belongs to rank 0 */
                    printf("%7.4f", v[line * lines + x * (interlines + 1)]);
                } else {
                    /* this line belongs to another rank and was received above
                     */
                    printf("%7.4f", v[x * (interlines + 1)]);
                }
            }
        }

        if (rank == 0) printf("\n");
    }
    fflush(stdout);
}
