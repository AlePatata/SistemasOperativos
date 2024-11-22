#include "nthreads.h"
#include "compartir.h"

// Defina aca sus variables globales
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t c = PTHREAD_COND_INITIALIZER;
void *p = NULL; // datos compartidos, datos divididos mi amor
int cont = 0;

void compartir(void *ptr) {
  pthread_mutex_lock(&m);

  while (cont>0) {  // yo no puedo compartir tus datos
    pthread_cond_wait(&c, &m);
  }
  p = ptr; // te comparto el engaño
  pthread_cond_broadcast(&c); // y comparto mis dias y el dolor
  
  while (p != NULL) {  // yo no puedo compartir tus datos
    pthread_cond_wait(&c, &m);
  }
  pthread_mutex_unlock(&m);
  return;
}

void *acceder(void) {
  pthread_mutex_lock(&m);
  while (p == NULL) { // yo no puedo compartir tus datos
    pthread_cond_wait(&c, &m);
  }
  cont++;
  pthread_mutex_unlock(&m);
  return p;
}

void devolver(void) { // oh mi amor
  pthread_mutex_lock(&m);
  cont--; // mi amor compartido
  if (cont == 0) {
    p = NULL; 
    pthread_cond_broadcast(&c);
  }
  pthread_mutex_unlock(&m);
  return;
}
