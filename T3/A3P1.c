#include <pthread.h>
#include "A3P1.h"

/* Con el tiempo la empresa creció y ahora tienen 8 camiones y por lo tanto
   Necesitan una nueva forma de manejar el transporte de estos para poder
   distribuir los camiones entree los encargos de carga.

   En particular se nos pide redefinir la función transporte, añadiendo funciones
   para:
      - poder buscar camiones disponibles
      - marcar que se está desocupando un camión

   La empresa nos dió lo siguiente para hacer el trabajo:
*/

#define P 8      // Número de camiones
#define R 100    // Número max de encargos
#define TRUE 1
#define FALSE 0

Camion *camiones[P];    // Los camiones
Ciudad *ubicaciones[P]; // Ciudades en las cuales se trabaja
double distancia(Ciudad * orig, Ciudad *dest); // dest entre 2 ciudades

/* Patron Request
 * 
 * Cada thread tiene su propia condicion para asi evitar 
 * despertar a todos los threads e ir llamando espeficicos 
 * threads a despertarse cuando corresponde
**/

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER; // herramienta de sincronización
int ocupados[P]; // pos i es TRUE si camión está ocupado

typedef struct {
    int ready;
    pthread_cond_t c;
    int idx_camion;  // info para que nosotros usemos el recurso
    Ciudad *ubicacion; // info relevante para el que ve las requests
} Request;

Request *requests[R]; // arreglo de peticiones


int buscar(Ciudad *orig){
    int idx_camion = -1;
    pthread_mutex_lock(&m);
    // ver si hay algún camion desocupado
    for(int i=0; i<P; i++){
        if(!ocupados[i]){ // si no esta ocupado podríamos ocuparlo
            // si es el primer camion libre asumiremos que lo podemos tomar
            // y si vemos otro camion veamos si está mas cerca
            if(idx_camion == -1 ||         
                distancia(ubicaciones[i] , orig) < distancia(ubicaciones[idx_camion], orig)){
                // si es el primero, o mas cercano será el nuestro
                idx_camion = i;
            }
        }
    }

    // posible tengamos algun camion, pero podria ser que todos esten ocupados
    if(idx_camion < 0){ // los camiones esta ocupados
        Request req = {FALSE, PTHREAD_COND_INITIALIZER, -1, orig};
        // guardar la peticion donde podamos en el arreglo de Requests
        for(int i=0; i<R; i++){
            if(requests[i] != NULL){
                requests[i] = &req;
                break;
            }
        }
        // esperamos
        while(!req.ready){
            pthread_cond_wait(&req.c, &m);
        }
        idx_camion = req.idx_camion;
    } 

    pthread_mutex_unlock(&m);
    return idx_camion;
}


void desocupar(int idx){
    pthread_mutex_lock(&m);
    
    int req_idx = -1;
    for(int i=0; i<R; i++){
        if (requests[i] != NULL){
            // si vemos un cliente, podemos pasarle el camion
            // si vemos 2 o mas clientes, se lo damos al mas cercano
            if(req_idx == -1 ||
                distancia(ubicaciones[idx] , requests[i]->ubicacion) 
                < distancia(ubicaciones[idx] , requests[req_idx]->ubicacion)
                ){
                req_idx = i;
            }
        }
    }
    // si hay cliente esperando, despertarlo
    if(req_idx >= 0){
        requests[req_idx]->ready = TRUE;
        requests[req_idx]->idx_camion = idx;
        pthread_cond_signal(&requests[req_idx]->c);
        requests[req_idx] = NULL;
    } else {
        // desocupar camion solo si no hay clientes esperando
        ocupados[idx] = FALSE;
    }

    pthread_mutex_unlock(&m);
}


void transportar(Contenedor *cont, Ciudad *orig, Ciudad *dest){
    pthread_mutex_lock(&m);
    // adquirir el camion
    int camion = buscar(orig);
    Ciudad ubic = ubicaciones[camion];
    pthread_mutex_unlock(&m);


    conducir(camion, ubic, orig);  
    cargar(camion, cont);          
    conducir(camion, orig, dest);  
    descargar(camion, cont);  

    // soltar el camion 
    pthread_mutex_lock(&m);  
    // liberar camion  
    ubicaciones[camion] = dest; // actualizar posicion del camion
    desocupar(camion);
    
    pthread_mutex_unlock(&m);
    
}
