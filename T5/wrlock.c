#define _XOPEN_SOURCE 500

#include "nthread-impl.h"

#include "rwlock.h"

struct rwlock {
 NthQueue *rq;
 NthQueue *wq;
 int r;
 int w;
};
void eliminar(nThread th){
  nth_delQueue(th->ptr, th);
  th->ptr=NULL;
}
nRWLock *nMakeRWLock() {
  nRWLock *rwl = (nRWLock *)nMalloc(sizeof(nRWLock));
  rwl->rq = nth_makeQueue();
  rwl->wq = nth_makeQueue();
  rwl->r = 0;
  rwl->w = 0;
  return rwl;
}

void nDestroyRWLock(nRWLock *rwl) {
  nth_destroyQueue(rwl->rq);
  nth_destroyQueue(rwl->wq);
  free(rwl);
}

int nEnterRead(nRWLock *rwl, int timeout) {
  START_CRITICAL
  nThread thisth = nSelf();
  if(rwl->w == 0 && nth_emptyQueue(rwl->wq)) {
    rwl->r++;
    END_CRITICAL
    return 1;
  }
  else { 
    thisth->ptr=rwl->rq;
    nth_putBack(rwl->rq, thisth);
     if (timeout>0){
      suspend(WAIT_RWLOCK_TIMEOUT);
      nth_programTimer((long long)timeout *1000000,&eliminar);
    }else{
      suspend(WAIT_RWLOCK);
    }
    schedule();
  }
  END_CRITICAL
  if (thisth->ptr == NULL){
    return 0;
    }
    else return 1;
}

int nEnterWrite(nRWLock *rwl, int timeout) {
  START_CRITICAL
  nThread thisth = nSelf();
  if(rwl->r == 0 && rwl->w == 0) {
    rwl->w++;
    END_CRITICAL
    return 1;
  }
  else {
    nth_putBack(rwl->wq, thisth);
    thisth->ptr=rwl->wq;
    if (timeout>0){
      suspend(WAIT_RWLOCK_TIMEOUT);
      nth_programTimer((long long)timeout *1000000,&eliminar);
    }else{
      suspend(WAIT_RWLOCK);
    }
    schedule();
  }
  END_CRITICAL
  if (thisth->ptr == NULL){
    return 0;
    }
    else return 1;
}

void nExitRead(nRWLock *rwl) {
  START_CRITICAL
  rwl->r--;
  if(rwl->r == 0 && !nth_emptyQueue(rwl->wq)) {
    nThread w= nth_getFront(rwl->wq);
    if(w->status == WAIT_RWLOCK_TIMEOUT){
        nth_cancelThread(w);
      }
    rwl->w++;
    setReady(w);
    schedule();
  }
  END_CRITICAL
}

void nExitWrite(nRWLock *rwl) {
  START_CRITICAL
  rwl->w--;
  if(!nth_emptyQueue(rwl->rq)) {
    while (!nth_emptyQueue(rwl->rq))
    {
      nThread r = nth_getFront(rwl->rq);
      if(r->status == WAIT_RWLOCK_TIMEOUT){
        nth_cancelThread(r);
      }
      rwl->r++;
      setReady(r);
      schedule();
    }
    
  }
  else if(!nth_emptyQueue(rwl->wq)) {
    nThread w = nth_getFront(rwl->wq);
    if(w->status == WAIT_RWLOCK_TIMEOUT){
        nth_cancelThread(w);
      }
    rwl->w++;
    setReady(w);
    schedule();
  }
  END_CRITICAL
}
