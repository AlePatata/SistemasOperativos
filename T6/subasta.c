#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "spinlocks.h"
#include "pss.h"
#include "subasta.h"

struct subasta {
  int n;  // unidades
  PriQueue *pq;
  int sl;
  int state;
};

// Defina aca otras estructuras si las necesita
typedef struct {
  int sl;
  int result;
} Request;


Subasta nuevaSubasta(int unidades) {
  Subasta s = (Subasta)malloc(sizeof(struct subasta));
  s->n = unidades;
  s->pq = makePriQueue();
  s->sl = OPEN;
  s->state = 1;
  return s;
}

void destruirSubasta(Subasta s) {
  if (s) {
    destroyPriQueue(s->pq);
    free(s);
  }
}

int ofrecer(Subasta s, double precio) {
  spinLock(&(s->sl));
  sleep(0);
  if (s->state) { // Si la subasta sigue abierta
    Request req = {CLOSED, 0}; // sl en wait, result 0 (no aceptado)

    // Si hay menos ofertas que unidades
    if (priLength(s->pq) < s->n) {
      priPut(s->pq, &req, (double) precio);
      spinUnlock(&(s->sl)); 
      spinLock(&req.sl); // wait
      return req.result;
    }
    else {
      // Si mi oferta es mejor que la menor de ellas
      if (priBest(s->pq) < precio) {
        Request *prev = priGet(s->pq);
        priPut(s->pq, &req, (double) precio);
        spinUnlock(&(prev->sl)); // despertamos al otro
        spinUnlock(&(s->sl)); 
        spinLock(&req.sl); // wait
        return req.result;
      }
      // Si no, ni siquiera me pongo a la cola
      else {
        spinUnlock(&(s->sl));
        return 0;
      }
    }
    
  } 
  spinUnlock(&(s->sl));
  return 0;
}

double adjudicar(Subasta s, int *prestantes) {
  spinLock(&(s->sl));
  s->state = 0; // Cerramos la subasta
  double rec = 0;
  while (priLength(s->pq) > s->n) {
    Request *req = priGet(s->pq);
    spinUnlock(&(req->sl));
  }
  
  while (!emptyPriQueue(s->pq) && (s->n)) {
    rec += (double) priBest(s->pq);
    Request *req = priGet(s->pq);
    req->result = 1;
    (s->n)--;
    spinUnlock(&(req->sl));
  }

  *prestantes = (s->n);
  spinUnlock(&(s->sl));
  return rec;
}