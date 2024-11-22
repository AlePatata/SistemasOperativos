#define _XOPEN_SOURCE 500

#include "nthread-impl.h"

#include "disk.h"

// Defina aca sus variables globales
int U;
PriQueue *gq, *lq;

void wakeUpFun(nThread nt) {
    priDel(gq, nt);
    priDel(lq, nt);
    nt->ptr = NULL;
}

void iniDisk(void) {
    gq = makePriQueue();
    lq = makePriQueue();
    U = -1;
    return;
}

void cleanDisk(void) {
    destroyPriQueue(gq);
    destroyPriQueue(lq);
    return;
}

int nRequestDisk(int track, int timeout) {
    START_CRITICAL
    nThread thisTh = nSelf();
    thisTh->ptr = &track;
    if (U == -1) { // Si está desocupada
        U = track;
    }
    else { 
        if (timeout > 0) {
        if (track >= U) {
          priPut(gq, thisTh, track);
            suspend(WAIT_REQUEST_TIMEOUT);
        }
        else {
          priPut(lq, thisTh, track);
            suspend(WAIT_REQUEST_TIMEOUT);
        }
          nth_programTimer(timeout*1000000LL, wakeUpFun);
      } else {
          if (track >= U) {
          priPut(gq, thisTh, track);
          suspend(WAIT_REQUEST);
          }
        else {
          priPut(lq, thisTh, track);
                suspend(WAIT_REQUEST);
            }
      }
      schedule();
    }
    END_CRITICAL    
    if(thisTh->ptr == NULL)
        return 1;
    return 0;
}

void nReleaseDisk() {
    START_CRITICAL
    nThread nt;
    int track;
    U = -1;
    if (!emptyPriQueue(gq)) {
      track = (int) priBest(gq);
      nt = priGet(gq);
      U = track;
      if (nt->status == WAIT_REQUEST_TIMEOUT) {
            nth_cancelThread(nt);
      }
      setReady(nt);
      schedule(); 
    } else {
        PriQueue *aux = gq;
        gq = lq;
        lq = aux;
        if (!emptyPriQueue(gq)) {
            track = (int) priBest(gq);
            nt = priGet(gq);
            U = track;
            if (nt->status == WAIT_REQUEST_TIMEOUT) {
              nth_cancelThread(nt);
        }
        setReady(nt);
        schedule();
        } 
    }
    END_CRITICAL
    return;
}
