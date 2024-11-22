#include "nthread-impl.h"
#include "nthread.h"
#include <pss.h>

/// ================== Version con timeout ===================

// usaremos la función
// nth_programTimer(tiempo_ns, arg_ignorado)
// para suspender el thread por tiempo_ns nanosegundos

// receive debe tener un timeout que lo suspenda mientras espera
void *nReceive(nThread *pth, int timeout){
    START_CRITICAL
    nThread this_th = nSelf();

    // Si no hay mensajes en cola
    if(nth_emptyQueue(this_th->sendQueue) && timeout_ms != 0){
        if(timeout_ms > 0){
            // programar timer
            suspend(WAIT_SEND_TIMEOUT)
            nth_programTimer(timeout_ms * 1000000, NULL);
        } else {
            // esperar a que llegue mensaje
            suspend(WAIT_SEND);
        }

        
        schedule();
    }

    // tomar 1er thread de la cola y retornar su mensaje
    nThread sender_th = nth_getFront(this_th->sendQueue);

    // si el thread estaba en timeout y se despierta, no hay mensaje
    // por lo que tenemos que retornar NULL
    *pth = sender_th ? sender_th : NULL;
    // *pth = [thread_que_envio_mensaje] si existe [thread_que_envio_mensaje] de no ser así [NULL];
    // (python): pth = sender_th if sender_th else None

    void *msg = sender_th->msg;

    END_CRITICAL
    return msg;
}

// tenemos que cancelar timeout del thread al que le enviamos mensaje si es que tiene
int nSend(nThread th, void *msg){
    START_CRITICAL

    // si el thread th esta en nRecieve, se pone en ready
    if(th->status == WAIT_SEND || th->status == WAIT_SEND_TIMEOUT){
        if(th->status == WAIT_SEND_TIMEOUT){
            // cancelar timer si tiene timer
            nth_cancelThread(th);
        }
        setReady(th);
    }
    
    nThread this_th = nSelf(); // me
    // ponerme en la cola de threads que quiere mandar un mensaje
    nth_putBack(th->sendQueue, this_th);
    this_th->msg = msg; // guardar mensaje

    // esperar respuesta
    suspend(WAIT_REPLY);
    schedule();

    int rc = this_th->rc;
    // retornar rc
    END_CRITICAL
    return rc;
}

// no hay cambios
void nReply(nThread th, int rc){
    START_CRITICAL
    
    th->rc = rc;
    setReady(th);
    scheduler();

    END_CRITICAL
}
