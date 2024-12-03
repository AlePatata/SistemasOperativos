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