/****************************************************************************/
/****************************************************************************/
/**                                                                        **/
/**  	      	   TU Muenchen - Institut fuer Informatik                  **/
/**                                                                        **/
/** Copyright: Dr. Thomas Ludwig                                           **/
/**            Thomas A. Zochler                                           **/
/**                                                                        **/
/** File:      askparams.c                                                 **/
/**                                                                        **/
/****************************************************************************/
/****************************************************************************/

/****************************************************************************/
/** Beschreibung der Funktion AskParams():                                 **/
/**                                                                        **/
/** Die Funktion AskParams liest sechs Parameter (Erkl"arung siehe unten)  **/
/** entweder von der Standardeingabe oder von Kommandozeilenoptionen ein.  **/
/**                                                                        **/
/** Ziel dieser Funktion ist es, die Eingabe der Parameter sowohl inter-   **/
/** aktiv als auch als Kommandozeilenparameter zu erm"oglichen.            **/
/**                                                                        **/
/** F"ur die Parameter argc und argv k"onnen direkt die vom System         **/
/** gelieferten Variablen der Funktion main verwendet werden.              **/
/**                                                                        **/
/** Beispiel:                                                              **/
/**                                                                        **/
/** int main (int argc, char **argv)                                       **/
/** {                                                                      **/
/**   ...                                                                  **/
/**   AskParams(..., argc, argv);                                          **/
/**   ...                                                                  **/
/** }                                                                      **/
/**                                                                        **/
/** Dabei wird argv[0] ignoriert und weiter eingegebene Parameter der      **/
/** Reihe nach verwendet.                                                  **/
/**                                                                        **/
/** Falls bei Aufruf von AskParams() argc < 2 "ubergeben wird, werden      **/
/** die Parameter statt dessen von der Standardeingabe gelesen.            **/
/****************************************************************************/
/** int *method;                                                           **/
/**         Bezeichnet das bei der L"osung der Poissongleichung zu         **/
/**         verwendende Verfahren (Gauss-Seidel oder Jacobi).              **/
/** Werte:  METH_GAUSS_SEIDEL  oder METH_JACOBI (definierte Konstanten)    **/
/****************************************************************************/
/** int *interlines:                                                       **/
/**         Gibt die Zwischenzeilen zwischen den auszugebenden             **/
/**         neun Zeilen an. Die Gesamtanzahl der Zeilen ergibt sich als    **/
/**         lines = 8 * (*interlines) + 9. Diese Art der Berechnung der    **/
/**         Problemgr"o"se (auf dem Aufgabenblatt mit N bezeichnet)        **/
/**         wird benutzt, um mittels der Ausgaberoutine DisplayMatrix()    **/
/**         immer eine "ubersichtliche Ausgabe zu erhalten.                **/
/** Werte:  0 < *interlines                                                **/
/****************************************************************************/
/** int *func:                                                             **/
/**         Bezeichnet die St"orfunktion (I oder II) und damit auch        **/
/**         die Randbedingungen.                                           **/
/** Werte:  FUNC_F0: f(x,y)=0, 0<x<1, 0<y<1                                **/
/**         FUNC_FPISIN: f(x,y)=2pi^2*sin(pi*x)sin(pi*y), 0<x<1, 0<y<1     **/
/****************************************************************************/
/** int *termination:                                                      **/
/**         Gibt die Art der Abbruchbedingung an.                          **/
/** Werte:  TERM_PREC: Abbruchbedingung ist die Genauigkeit der bereits    **/
/**                 berechneten N"aherung. Diese soll unter die            **/
/**                 Grenze term_precision kommen.                          **/
/**         TERM_ITER: Abbruchbedingung ist die Anzahl der Iterationen.    **/
/**                 Diese soll gr"o"ser als term_iteration sein.           **/
/****************************************************************************/
/** double *term_precision:                                                **/
/** int t*erm_iteration:                                                   **/
/**         Es wird jeweils nur einer der beiden Parameter f"ur die        **/
/**         Abbruchbedingung eingelesen.                                   **/
/****************************************************************************/

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>

#include "partdiff-par.h"

static void usage(char* name) {
    printf("Usage: %s [num] [method] [lines] [func] [term] [prec/iter]\n",
           name);
    printf("\n");
    printf("  - num:       number of threads (1 .. %d)\n", MAX_THREADS);
    printf("  - method:    calculation method (1 .. 2)\n");
    printf("                 %1d: Gauß-Seidel\n", METH_GAUSS_SEIDEL);
    printf("                 %1d: Jacobi\n", METH_JACOBI);
    printf("  - lines:     number of interlines (0 .. %d)\n", MAX_INTERLINES);
    printf("                 matrixsize = (interlines * 8) + 9\n");
    printf("  - func:      interference function (1 .. 2)\n");
    printf("                 %1d: f(x,y) = 0\n", FUNC_F0);
    printf(
        "                 %1d: f(x,y) = 2 * pi^2 * sin(pi * x) * sin(pi * y)\n",
        FUNC_FPISIN);
    printf("  - term:      termination condition ( 1.. 2)\n");
    printf("                 %1d: sufficient precision\n", TERM_PREC);
    printf("                 %1d: number of iterations\n", TERM_ITER);
    printf("  - prec/iter: depending on term:\n");
    printf("                 precision:  1e-4 .. 1e-20\n");
    printf("                 iterations:    1 .. %d\n", MAX_ITERATION);
    printf("\n");
    printf("Example: %s 1 2 100 1 2 100 \n", name);
}

static int check_number(struct options* options) {
    return (options->number >= 1 && options->number <= MAX_THREADS);
}

static int check_method(struct options* options) {
    return (options->method == METH_GAUSS_SEIDEL ||
            options->method == METH_JACOBI);
}

static int check_interlines(struct options* options) {
    return (options->interlines <= MAX_INTERLINES);
}

static int check_inf_func(struct options* options) {
    return (options->inf_func == FUNC_F0 || options->inf_func == FUNC_FPISIN);
}

static int check_termination(struct options* options) {
    return (options->termination == TERM_PREC ||
            options->termination == TERM_ITER);
}

static int check_term_precision(struct options* options) {
    return (options->term_precision >= 1e-20 &&
            options->term_precision <= 1e-4);
}

static int check_term_iteration(struct options* options) {
    return (options->term_iteration >= 1 &&
            options->term_iteration <= MAX_ITERATION);
}

void AskParams(struct options* options, int argc, char** argv, int rank) {
    int ret;

    if (rank == 0) {
        printf(
            "============================================================\n");
        printf(
            "Program for calculation of partial differential equations.  \n");
        printf(
            "============================================================\n");
        printf("(c) Dr. Thomas Ludwig, TU München.\n");
        printf("    Thomas A. Zochler, TU München.\n");
        printf("    Andreas C. Schmidt, TU München.\n");
        printf(
            "============================================================\n");
        printf("\n");
    }

    if (argc < 2) {
        /* ----------------------------------------------- */
        /* Get input: method, interlines, func, precision. */
        /* ----------------------------------------------- */
        do {
            printf("\n");
            printf("Select number of threads:\n");
            printf("Number> ");
            fflush(stdout);
            ret = scanf("%" SCNu64, &(options->number));
            while (getchar() != '\n')
                ;
        } while (ret != 1 || !check_number(options));

        do {
            printf("\n");
            printf("Select calculationmethod:\n");
            printf("  %1d: Gauss-Seidel.\n", METH_GAUSS_SEIDEL);
            printf("  %1d: Jacobi.\n", METH_JACOBI);
            printf("method> ");
            fflush(stdout);
            ret = scanf("%" SCNu64, &(options->method));
            while (getchar() != '\n')
                ;
        } while (ret != 1 || !check_method(options));

        do {
            printf("\n");
            printf("Matrixsize = Interlines*8+9\n");
            printf("Interlines> ");
            fflush(stdout);
            ret = scanf("%" SCNu64, &(options->interlines));
            while (getchar() != '\n')
                ;
        } while (ret != 1 || !check_interlines(options));

        do {
            printf("\n");
            printf("Select interferencefunction:\n");
            printf(" %1d: f(x,y)=0.\n", FUNC_F0);
            printf(" %1d: f(x,y)=2pi^2*sin(pi*x)sin(pi*y).\n", FUNC_FPISIN);
            printf("interferencefunction> ");
            fflush(stdout);
            ret = scanf("%" SCNu64, &(options->inf_func));
            while (getchar() != '\n')
                ;
        } while (ret != 1 || !check_inf_func(options));

        do {
            printf("\n");
            printf("Select termination:\n");
            printf(" %1d: sufficient precision.\n", TERM_PREC);
            printf(" %1d: number of iterationes.\n", TERM_ITER);
            printf("termination> ");
            fflush(stdout);
            ret = scanf("%" SCNu64, &(options->termination));
            while (getchar() != '\n')
                ;
        } while (ret != 1 || !check_termination(options));

        if (options->termination == TERM_PREC) {
            do {
                printf("\n");
                printf("Select precision:\n");
                printf("  Range: 1e-4 .. 1e-20.\n");
                printf("precision> ");
                fflush(stdout);
                ret = scanf("%lf", &(options->term_precision));
                while (getchar() != '\n')
                    ;
            } while (ret != 1 || !check_term_precision(options));

            options->term_iteration = MAX_ITERATION;
        } else if (options->termination == TERM_ITER) {
            do {
                printf("\n");
                printf("Select number of iterations:\n");
                printf("  Range: 1 .. %d.\n", MAX_ITERATION);
                printf("Iterations> ");
                fflush(stdout);
                ret = scanf("%" SCNu64, &(options->term_iteration));
                while (getchar() != '\n')
                    ;
            } while (ret != 1 || !check_term_iteration(options));

            options->term_precision = 0;
        }
    } else {
        if (argc < 7 || strcmp(argv[1], "-h") == 0 ||
            strcmp(argv[1], "-?") == 0) {
            usage(argv[0]);
            exit(0);
        }

        ret = sscanf(argv[1], "%" SCNu64, &(options->number));

        if (ret != 1 || !check_number(options)) {
            usage(argv[0]);
            exit(1);
        }

        ret = sscanf(argv[2], "%" SCNu64, &(options->method));

        if (ret != 1 || !check_method(options)) {
            usage(argv[0]);
            exit(1);
        }

        ret = sscanf(argv[3], "%" SCNu64, &(options->interlines));

        if (ret != 1 || !check_interlines(options)) {
            usage(argv[0]);
            exit(1);
        }

        ret = sscanf(argv[4], "%" SCNu64, &(options->inf_func));

        if (ret != 1 || !check_inf_func(options)) {
            usage(argv[0]);
            exit(1);
        }

        ret = sscanf(argv[5], "%" SCNu64, &(options->termination));

        if (ret != 1 || !check_termination(options)) {
            usage(argv[0]);
            exit(1);
        }

        if (options->termination == TERM_PREC) {
            ret = sscanf(argv[6], "%lf", &(options->term_precision));
            options->term_iteration = MAX_ITERATION;

            if (ret != 1 || !check_term_precision(options)) {
                usage(argv[0]);
                exit(1);
            }
        } else {
            ret = sscanf(argv[6], "%" SCNu64, &(options->term_iteration));
            options->term_precision = 0;

            if (ret != 1 || !check_term_iteration(options)) {
                usage(argv[0]);
                exit(1);
            }
        }
    }
}
