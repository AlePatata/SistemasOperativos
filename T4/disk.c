#define _XOPEN_SOURCE 500
#include "nthread-impl.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "disk.h"


// Defina aca tipos y variables globales
int U;
PriQueue *gq, *lq;

void iniDisk(void) {
  START_CRITICAL

  gq = makePriQueue();
  lq = makePriQueue();
  U = -1;
  END_CRITICAL

  return;
}

void cleanDisk(void) {
  START_CRITICAL

  destroyPriQueue(gq);
  destroyPriQueue(lq);

  END_CRITICAL

  return;
}

void nRequestDisk(int track, int delay) {
  START_CRITICAL

  nThread thisTh = nSelf();
  if (U == -1) { // Si está desocupada
    U = track;
    //setReady(thisTh);
    schedule();
  } else {
    if (track >= U) {
      priPut(gq, thisTh, track);
      suspend(WAIT_SEM);
      schedule();
    } else {
      priPut(lq, thisTh, track);
      suspend(WAIT_SEM);
      schedule();
    }
    U = track;
  }
  END_CRITICAL

  return;
}

void nReleaseDisk() {
  START_CRITICAL

  nThread nt;
  double track;
  U = -1;
  if (!emptyPriQueue(gq)) {
    track = priBest(gq);
    nt = priGet(gq);
    priPeek(gq);
    U = track;
    setReady(nt);
    schedule(); 
  } else {
    PriQueue *aux = gq;
    gq = lq;
    lq = aux;
    if (!emptyPriQueue(gq)) {
      track = priBest(gq);
      nt = priGet(gq);
      priPeek(gq);
      U = track;
      setReady(nt);
      schedule();
    } 
    //else 
    //  return;
  }
  END_CRITICAL

  return;
}
