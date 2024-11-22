#include <stdio.h>
#include <pthread.h>

#include "pss.h"
#include "pedir.h"


// Defina aca sus variables globales
pthread_mutex_t m;
int zeros, ones;
Queue *oq; // Cola de requests de categoria 1
Queue *zq; // Cola de requests de categoria 0

typedef struct {
  int ready;
  int cat;
  pthread_cond_t cond;
} Request;

void iniciar() {
  pthread_mutex_init(&m, NULL);
  pthread_mutex_lock(&m);
  oq = makeQueue();
  zq = makeQueue();
  zeros = 0;
  ones = 0;
  pthread_mutex_unlock(&m);
}

void terminar() {
  pthread_mutex_lock(&m);
  destroyQueue(oq);
  destroyQueue(zq);
  pthread_mutex_unlock(&m);
  pthread_mutex_destroy(&m);
}

void pedir(int cat) {
  pthread_mutex_lock(&m);
  Request req = {0,cat,PTHREAD_COND_INITIALIZER};
  if (cat) { // Si lo pide un 1
    if (!zeros && !ones) // Si no hay nadie usandolo
      ones = 1;
    else {
      put(oq, &req); // Else lo ponemos a la cola de 1's
      while(!req.ready){
        pthread_cond_wait(&req.cond, &m);
      }
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
  pthread_mutex_unlock(&m);
  return;
}

void devolver() {
  pthread_mutex_lock(&m);
  Request *req = NULL;
  if (ones) { // Si el recurso fue ocupado por un 1
    ones = 0;
    if (!emptyQueue(zq)) { // Hay 0s esperando?
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
  
  pthread_mutex_unlock(&m);
  return;
}