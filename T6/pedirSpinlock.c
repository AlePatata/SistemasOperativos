#include <stdio.h>
#include <pthread.h>

#include "pss.h"
#include "pedir.h"


// Defina aca sus variables globales
int spinlock = OPEN
int zeros, ones;
Queue *oq; // Cola de requests de categoria 1
Queue *zq; // Cola de requests de categoria 0

/* No necesitamos estructura pues el spinlock guarda toda la info necesaria
(necesita esperar y ser despertado)

typedef struct {
  int wait;
} Request;
*/

void iniciar() {
  oq = makeQueue();
  zq = makeQueue();
  zeros = 0;
  ones = 0;
}

void terminar() {
  destroyQueue(oq);
  destroyQueue(zq);
}

void pedir(int cat) {
  SpinLock(&spinlock);
  int sl = CLOSED;
  if (cat) { // Si lo pide un 1
    if (!zeros && !ones) // Si no hay nadie usandolo
      ones = 1;
    else {
      put(oq, &sl); // Else lo ponemos a la cola de 1's
      /*while(!sl){
        pthread_cond_wait(&sl, &m);
      }*/
      ones = 1;
    }
  }
  else { // Si lo pide un 0
    if (!zeros && !ones) // Si no hay nadie usandolo
      zeros = 1;
    else {
      put(zq, &req); // Else lo ponemos a la cola de 0's
      while(!req.ready) {
        pthread_cond_wait(&req.cond, &m);
      }
      zeros = 1;
    } 
  }
  
  SpinUnlock(&spinlock);
  return;
}

void devolver() {
  SpinLock(&spinlock);
  Request *req = NULL;
  if (ones) { // Si el recurso fue ocupado por un 1
    ones = 0;
    if (!emptyQueue(zq)) { // Hay 0s esperando?
      int *psl;
      req = peek(zq);
      if (req) { // Si la request está bien definida
        req->ready = 1; // Será la siguiente en ser atendida
        zeros = 1;
        pthread_cond_signal(&req->cond); 
      } 
      get(zq);
    }
    else if (!emptyQueue(oq)) { // Hay 1s esperando?
      req = peek(oq);
      if (req) { // Si la request está bien definida
        req->ready = 1; // Será la siguiente en ser atendida
        ones = 1;
        pthread_cond_signal(&req->cond); 
      } 
      get(oq);
    }
  } 
  else if (zeros) { // Si el recurso fue ocupado por un 0
    zeros = 0;
    if (!emptyQueue(oq)) {// Hay 1s esperando?
      req = peek(oq);
      if (req) { // Si la request está bien definida
        req->ready = 1; // Será la siguiente en ser atendida
        ones = 1;
        pthread_cond_signal(&req->cond); 
      } 
      get(oq);
    }
    else if (!emptyQueue(zq)) { // Hay 0s esperando?
      req = peek(zq);
      if (req) { // Si la request está bien definida
        req->ready = 1; // Será la siguiente en ser atendida
        zeros = 1;
        pthread_cond_signal(&req->cond); 
      } 
      get(zq);
    }
  }
  
  
  SpinUnlock(&spinlock);
  return;
}