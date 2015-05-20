/****************************************************************************/
/****************************************************************************/
/**                                                                        **/
/**  	      	   TU Muenchen - Institut fuer Informatik                  **/
/**                                                                        **/
/** Copyright: Prof. Dr. Thomas Ludwig                                     **/
/**            Thomas A. Zochler, Andreas C. Schmidt                       **/
/**                                                                        **/
/** File:      partdiff-seq.h                                              **/
/**                                                                        **/
/****************************************************************************/
/****************************************************************************/

/* *********************************** */
/* Include some standard header files. */
/* *********************************** */
#include <math.h>
#include <stdint.h>

/* ************* */
/* Some defines. */
/* ************* */
#ifndef PI
#define PI 3.141592653589793
#endif
#define TWO_PI_SQUARE (2 * PI * PI)
#define MAX_INTERLINES (int)10240
#define MAX_ITERATION (int)200000
#define MAX_THREADS (int)1024
#define METH_GAUSS_SEIDEL (int)1
#define METH_JACOBI (int)2
#define FUNC_F0 (int)1
#define FUNC_FPISIN (int)2
#define TERM_PREC (int)1
#define TERM_ITER (int)2

struct options {
    uint64_t number;      /* Number of threads                              */
    uint64_t method;      /* Gauss Seidel or Jacobi method of iteration     */
    uint64_t interlines;  /* matrix size = interlines*8+9                   */
    uint64_t inf_func;    /* inference function                             */
    uint64_t termination; /* termination condition                          */
    uint64_t term_iteration; /* terminate if iteration number reached */
    double term_precision; /* terminate if precision reached                 */
};

/* *************************** */
/* Some function declarations. */
/* *************************** */
/* Documentation in files      */
/* - askparams.c               */
/* - displaymatrix.c           */
/* *************************** */
void AskParams(struct options*, int, char**, int rank);
