#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <sys/time.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>
#include <pthread.h>

#include "viajante.h"


// Defina aca las estructuras y funciones adicionales que necesite
// No defina variables globales, no las necesita

typedef struct {
    int *z;
    int n;
    double **m; 
    int nperm;
    double sol;
} Args;

void *viajante_thread(void *ptr) {
    Args *arg = (Args *) ptr;
    arg -> sol = viajante(arg -> z, arg -> n, arg -> m, arg -> nperm);
    return NULL;
}

double viajante_par(int z[], int n, double **m, int nperm, int p) {
    pthread_t t[p];
    Args array[p];
    for (int i = 0; i<p; i++) {
        Args *arg = &array[i];
        arg -> z = (int *)malloc(sizeof(int)*(n+1));
        arg -> n = n;
        arg -> m = m;
        int a = nperm/p;
        arg -> nperm = (i<nperm%p)? a+1 : a;
        pthread_create(&t[i], NULL, viajante_thread, arg);
    }
    double min = DBL_MAX;
    for (int i = 0; i < p; i++){
        pthread_join(t[i], NULL);
        Args *arg = (Args *) &array[i];
        double min_thread_i = arg -> sol;
        if (min_thread_i < min) {
            min = min_thread_i;
            for (int j = 0; j < n+1; ++j) {
                z[j] = (arg -> z)[j];
            }
        }
        free(arg->z);
    }
    return min;
}
