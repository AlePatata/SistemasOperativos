#include <semaphore.h>

enum { ROJO = 0, AZUL = 1 };
int cantidad[2] = {0, 0};

sem_t mutex_entrada_baño;    // sem_init(&mutex_entrada_baño, 0, 1);
sem_t mutex_zona_crítica[2]; // sem_init(&mutex_zona_crítica[ROJO], 0, 1);
                             // sem_init(&mutex_zona_crítica[AZUL], 0, 1);

/* En esta solución el mutex representa el acceso al baño. Al igual que
 * en la implementación incorrecta del enunciado, el mutex está en posesión
 * del equipo que está adentro (y queda disponible cuando el baño está
 * desocupado).
 *
 * Los dos semáforos adicionales (la variable 'mutex_zona_crítica') protegen la
 * entrada a la zona crítica (la variable 'cantidad' de cada color) y además
 * sirven para "poner en cola" a los hinchas. Notar que las zonas críticas están
 * separadas para cada color.
 */

void entrar(int color) {
  sem_wait(&mutex_zona_crítica[color]); // inicio de sección crítica.

  if (cantidad[color] == 0)
    sem_wait(&mutex_entrada_baño); // el primero en entrar toma el mutex
  /* NOTA: aquí NO se debe llamar a sem_post(&mutex_zona_crítica[color]) antes
   * de tomar el mutex. De lo contrario, los hinchas del mismo equipo que entren
   * después se quedaran esperando en esta linea (si el baño esta ocupado).
   * Luego, cuando el otro equipo suelte el mutex sólo uno de los que estaba
   * esperando podrá entrar. En su lugar, los hinchas deben "hacer cola" en el
   * mutex mutex_zona_crítica[color], al inicio de esta función.
   */

  cantidad[color]++;

  sem_post(&mutex_zona_crítica[color]); // fin de la sección crítica
}

void salir(int color) {
  sem_wait(&mutex_zona_crítica[color]); // inicio de la sección crítica

  cantidad[color]--;
  if (cantidad[color] == 0)
    sem_post(&mutex_entrada_baño); // el último en salir suelta el mutex

  sem_post(&mutex_zona_crítica[color]); // fin de la sección crítica
}
