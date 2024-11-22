#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>

#include "viajante.h"

// Defina aca las estructuras y funciones adicionales que necesite
// No defina variables globales, no las necesita

double DBL_MAX = 1.79769e+308;

typedef struct {
    int *z;
    int n;
    double **m; 
    int nperm;
    double sol;
} Args;

double viajante(int z[], int n, double **m, int nperm) {
    double min = DBL_MAX; // la menor distancia hasta el momento
    for (int i = 1; i<=nperm; i++) {
        int x[n+1]; // almacenará una ruta aleatoria
        gen_ruta_alea(x, n);
        double d = dist(x, n, m);
        if (d<min) { // si 
            min = d; // d es la nueva menor distancia
            for (int j= 0; j<=n; j++)
                z[j] = x[j]; // guarda ruta
        } 
    }
    return min;
}

void *viajante_thread(void *ptr) {
    Args *arg = (Args *) ptr;
    arg -> sol = viajante(arg -> z, arg -> n, arg -> m, arg -> nperm);
    return NULL;
}


double viajante_par(int z[], int n, double **m, int nperm, int p) {
    pthread_t t[p];
    Args* array[p];
    for (int i = 0; i<p; i++) {
        Args *arg = array[i];
        arg -> z = z;
        arg -> n = n;
        arg -> m = m;
        arg -> nperm = nperm/p;
        pthread_create(&t[i], NULL, viajante_thread, arg);
    }
    double min = DBL_MAX;
    for (int i = 0; i < p; i++){
        pthread_join(t[i], NULL);
        double min_thread_i = array[i] -> sol;
        if (min_thread_i < min) 
            min = min_thread_i;
    }
    return min;
}
