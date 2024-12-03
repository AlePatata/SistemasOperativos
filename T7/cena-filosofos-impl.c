/*
# Explicación de mi tarea

## Buffer
Definí el buffer como un string donde cada fila siempre tiene 12 caracteres contando el '\n', para el caso de un filósofo esperando esto se logra de manera natural concatenando el numero del filosofo, espacio, "esperando\n" y \n; Pero para las acciones de comiendo y pensado se debe agregar un espacio extra ("i"+" "+ "comiendo"/"pensando" +" "+'/n'). Así, el buffer toma la siguiente forma: \
*inicialmente:* \
"0 pensando \n1 pensando \n2 pensando \n3 pensando \n4 pensando \n" \
*ejemplo forma general* \
"0 pensando \n1 comiendo \n2 esperando\n3 comiendo \n4 pensando \n" \

## Patrón Request
Usé una estructura Request que guarda el numero del filosofo "fil", la condición del filósofo y un campo ready que indicaría cuando le asignaran comer. Esto solo se puede llevar a cabo usando colas por lo que debí implementarlas en las funciones auxiliares.
En la función write se aplica el pedir (comer) y devolver (pensar), teniendo en cuenta las interrupciones cuando c_wait retorne -EINTR se evaluará si darle el palillo al filosofo si su estado ready esta en 0, si no se considera que éste se arrepintió e interrumpió el proceso. Esto lo hice así porque no pude encontrar una buena implementación de "delPri()" pero para una queue, que sacara la request de la cola sin importar dónde esté. 


## Funciones auxiliares
- compStr(char * s1, char * s2): compara 2 strings, para este caso siempre usaremos s2 como una de las posibles acciones del filósofo y s1 como lo que el usuario escribirá en la shell 
- updateBuffer(int index, char * s2): modifica el buffer, que considerando como lo definimos, dado el numero de un filosofo (index) y el string s2 
- puede(int i) pregunta si el filosofo i puede comer
- Todas las necesarias para la implementación de un a cola, sacadas de pss.c. Además se debió usar kmalloc y kfree en vez de malloc y free, y para reemplazar la función sizeof() usé un tamaño constante de 1000. Las funciones serían las siguientes: 
+ makeQueue()
+ put(Queue *q, void *ptr)
+ get(Queue *q)
+ emptyQueue(Queue *q)
+ peek()
+ destroyQueue(Queue *q)
*/

/* Necessary includes for device drivers */
#include <linux/init.h>
/* #include <linux/config.h> */
#include <linux/module.h>
#include <linux/kernel.h> /* printk() */
#include <linux/slab.h> /* kmalloc() */
#include <linux/fs.h> /* everything... */
#include <linux/errno.h> /* error codes */
#include <linux/types.h> /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h> /* O_ACCMODE */
#include <linux/uaccess.h> /* copy_from/to_user */

#include "kmutex.h"

MODULE_LICENSE("Dual BSD/GPL");

// Firmas
int filosofo_open(struct inode *inode, struct file *filp);
int filosofo_release(struct inode *inode, struct file *filp);
ssize_t filosofo_read(struct file *filp, char *buf, size_t count, loff_t *f_pos);
ssize_t filosofo_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);
void filosofo_exit(void);
int filosofo_init(void);

struct file_operations filosofo_fops = {
	.read = filosofo_read,
	.write = filosofo_write,
	.open = filosofo_open,
	.release = filosofo_release
};

module_init(filosofo_init);
module_exit(filosofo_exit);

// Varibles Globales y Buffer

#define FILOSOFO_MAJOR 61
#define MAX_SIZE 8192
#define N 5	 	 // Cantidad de filosofos
#define TOTAL 60 // Tamaño del buffer bien definido

typedef enum {
	pensando, 
	comiendo, 
	esperando
} state;

typedef struct {
	int fil;
	KCondition * c;
	int ready;
} Request;

static state filosofos[N];
static char * buffer;
static ssize_t curr_size;

static KMutex m;

//==========================================
// Implementacion de Queue
//==========================================

typedef struct node {
    void *val;
    struct node *next;
} QueueNode;

struct queue {
    QueueNode *first, **plast;
    int len;
};

typedef struct queue Queue;

Queue * makeQueue(void) {
    Queue *q = kmalloc(1000, GFP_KERNEL);
    q->first = NULL;
    q->plast = &q->first;
    q->len = 0;
    return q;
}

void put(Queue *q, void *ptr) {
    QueueNode *node = kmalloc(1000, GFP_KERNEL);
    node->val = ptr;
    node->next = NULL;
    *q->plast = node;
    q->plast = &node->next;
    q->len++;
}

void *get(Queue *q) {
    if (q->first == NULL) {
        return NULL;
    }
    void *val = q->first->val;
    QueueNode *next = q->first->next;
    kfree(q->first);
    q->first = next;
    if (next == NULL) {
        q->plast = &q->first;
    }
    q->len--;
    return val;
}

void *peek(Queue *q) {
    if (q->first == NULL) {
        return NULL;
    } else {
        return q->first->val;
    }
}

int emptyQueue(Queue *q) {
    return q->first == NULL;
}

void destroyQueue(Queue *q) {
    QueueNode *node = q->first;
    while (node != NULL) {
        QueueNode *next = node->next;
        kfree(node);
        node = next;   
    }
    kfree(q);
}

int compStr(char * s1, char * s2) {
	while (*s1 && *s2) {
        if (*s1 != *s2) {
            return 0; // No son iguales
        }
        s1++;
        s2++;
    }
    // Ambas strings deben terminar al mismo tiempo para ser iguales
    return (*s1 == '\0' && *s2 == '\0');
}

void updateBuffer(int index, char * new_state) {
	int begin = index*12 + 2; // vamos a la linea index
	snprintf(buffer + begin, 9, "%s\n", new_state);
}

int puede(int i) {
    return filosofos[i] != comiendo && 
           filosofos[(i + 1) % N] != comiendo &&
           filosofos[(i + N-1)%N] != comiendo;
}

Queue *q;

// Implentacion de las funciones
int filosofo_init(void) {
	int rc; // Código de Retorno
	rc = register_chrdev(FILOSOFO_MAJOR, "cena-filosofos", &filosofo_fops);
	if (rc < 0) { // si tratamos de registrar y retorno algo negativo, el major fue ocupado
		printk("filosofo_init error");
		return rc;
	}

	m_init(&m);
	q = makeQueue();

	buffer = kmalloc(MAX_SIZE, GFP_KERNEL);
	if (buffer == NULL) { // Si no habia memoria y el buffer no se pudo crear
		filosofo_exit(); // Limpiamos
		return -ENOMEM; // Retornamos codigo de ese tipo de error
	}

	// Estado de los filosofos inicialmente
	for (int i = 0; i < N; ++i) {
		filosofos[i] = pensando;
	}
	char * s = "0 pensando \n1 pensando \n2 pensando \n3 pensando \n4 pensando \n";
	int i;
	for (i = 0; s[i] != '\0'; i++) {
	    buffer[i] = s[i];
	} buffer[i] = '\0';

	printk(KERN_INFO "Buffer inicializado: %s\n", buffer);

	curr_size = 0;
	return 0;
}

void filosofo_exit(void) {
	unregister_chrdev(FILOSOFO_MAJOR, "cena-filosofos");
	if (buffer) { // Si el buffer no se liberó
		kfree(buffer);
	}
}

// filp: file descriptor 
int filosofo_open(struct inode *inode, struct file *filp) { 
	return 0;
} 

int filosofo_release(struct inode *inode, struct file *filp) {
	return 0;
}

ssize_t filosofo_read(struct file *filp, char *buf, size_t count, loff_t *f_pos) {
    ssize_t rc = 0;

    m_lock(&m);

    // Validar posición actual dentro del buffer
    if (*f_pos >= TOTAL) {
        rc = 0; // Nada más que leer
        goto epilog;
    }

    // Ajustar el tamaño de lectura al espacio disponible
    if (count > TOTAL - *f_pos) {
        count = TOTAL - *f_pos;
    }
    int e = copy_to_user(buf, buffer + *f_pos, count);
    if (e != 0) {
        printk(KERN_ERR "copy_to_user error\n");
        rc = -EFAULT;
        goto epilog;
    }

    *f_pos += count; // Actualizar el puntero de archivo
    rc = count;

epilog:
    m_unlock(&m);
    return rc;
}


// filp: filedescriptor, buf: donde quiere escribir el usuario
ssize_t filosofo_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos) {
    m_lock(&m);

    char user_input[32]; // Buffer temporal para comandos
    char instruccion[8]; // Almacena "comer" o "pensar"
    int index;
    ssize_t rc = 0;

    if (copy_from_user(user_input, buf, count)) {
        printk(KERN_ERR "Error al copiar desde el espacio de usuario.\n");
        rc = -EFAULT;
        goto epilog;
    } user_input[count] = '\0';

    // Recuperamos la instruccion y el indice del filosofo:
    int i = 0;
    int j = 0;
    while (user_input[i] != ' ' && user_input[i] != '\0' && j < sizeof(instruccion) - 1) {
        instruccion[j++] = user_input[i++];
    } instruccion[j] = '\0';

    // Si no encontramos un espacio o la instrucción está vacía
    if (user_input[i] != ' ' || j == 0) {
        printk(KERN_ERR "Formato inválido: '%s'\n", user_input);
        rc = -EINVAL;
        goto epilog;
    }
    i++; // Saltamos el espacio

    // Aplicamos una transformacion para recuperar el indica
    if (user_input[i] >= '0' && user_input[i] <= '9') {
        index = user_input[i] - '0'; // Pues segun el ascii '2'-'0' = 2
    } else {
        printk(KERN_ERR "Índice inválido: '%s'\n", &user_input[i]);
        rc = -EINVAL;
        goto epilog;
    }

    if (index < 0 || index >= N) {
        printk(KERN_ERR "Índice inválido: %d\n", index);
        rc = -EINVAL;
        goto epilog;
    }

    if (compStr(instruccion, "comer")) {
        if (filosofos[(index + N-1)%N] != esperando &&
            filosofos[(index + 1)%N] != esperando &&
            puede(index)) { // si sus vecinos no estan no estan esperando ni comiendo
            filosofos[index] = comiendo;
            updateBuffer(index, "comiendo \n");
        } else {
            filosofos[index] = esperando;
            updateBuffer(index, "esperando\n");
            KCondition my_c;
            c_init(&my_c);
            Request req = {index, &my_c, 0};
            put(q, &req);
            while (!req.ready) { // mientras no este listo
                if (c_wait(req.c, &m) == -EINTR) { // si fue interruptido
                    printk(KERN_INFO "Filósofo %d interrumpido. Vuelve a pensar.\n", index);
                    filosofos[index] = pensando;
                    updateBuffer(index, "pensando \n");
                    req.ready = 1;
                    rc = -EINTR;
                    goto epilog;
                }
            }
        }
    } else if (compStr(instruccion, "pensar")) {
        filosofos[index] = pensando;
        updateBuffer(index, "pensando \n");
        while (!emptyQueue(q)) {
            Request *req = peek(q);
            if (req->ready == 1) { // Si fue interrruptido
                get(q);
                c_signal(req->c);
            }
            if (puede(req->fil)) {
                get(q);
                filosofos[req->fil] = comiendo;
                updateBuffer(req->fil, "comiendo \n");
                req->ready = 1;
                c_signal(req->c);
            }
            else {
            	break;
        	}
        }
    } else {
        printk(KERN_ERR "Instrucción desconocida: '%s'\n", instruccion);
        rc = -EINVAL;
        goto epilog;
    }

    rc = count;

	epilog:
	    m_unlock(&m);
	    return rc;
}
