# Entrega System calls
## Estudiante
Archibald Emmanuel Carrion Claeys
## Descripcion
Esta segunda entrega del proyecto basado en el simulador de sistemas operativos NachOS, se tuvo que desarrollar los llamadas al sistema junto con una multitud de otros ambitos necesarios para su correcto funcionamiento.

## Llamadas al sistema y su implementacion
Es importante notar que no se logro implementar de manera functional varios llamados al sistema, por lo que se procedera a explicar los que si funcionan de manera correcta y los que tienen una estructura logica pero no se lograron implementar por problemas de compatibilidad con el simulador NachOS.

### Fork
El llamado al sistema Fork fue uno de los mas complejos de implementar ya que occupa dos funciones addicionales para su correcto funcionamiento, la function auxiliar **AuxFork** que se llama mediante la function fork del mismo thread creado en **NachOS_Fork**. 
Esa function auxiliar consiste en initializar los registros del nuevo thread, y modificar sus valores de los registros program counter.
La segunda function necesaria para el funcionamiento del Fork, es el constructor de la clase **AddrSpace** que recibe otro puntero hacia la clase AddrSpace, y copia los valores de los segmentos de memoria compartida entre ambos threads, para que el nuevo thread tenga acceso a la misma informacion que el thread padre. Pero ademas de eso tambien crea la memoria privada del nuevo thread.

### Modification de la class AddrSpace
Para poder implementar el llamado al sistema Fork, fue necesario modificar la clase AddrSpace, ya que se tuvo que agregar un nuevo construcot que recibe por parametro un puntero hacia la clase AddrSpace. 
De otro lado, al implementar los llamados al sistema tuve muchos problemas de segmentation fault, los cual interprete que podian ser provocados por un Thread que usaba memoria privada de otro thread, por lo que implemente una mapeo de memoria como mencionado por el profe el clase como el "MiMapita" que basandose en un BitMap de NachOS, permite saber cuales segmentos estan libres y cuales no.

Tras la implementacion de este mapeo de memoria, se logro solucionar algunos de los problemas.

### processTable
Al momento de implementar los llamados al sistema "Join" y "Exit" encontre el problema de la sincronizacion de los threads, ya que cuando un thread hace join a otro thread, tiene que esperar hasta que el otro termine.
Para implementar eso tuve que agregar al programa un array de punteros hacia instancias de struct processData que poseen dos campos, un semaphoro y un int que contiene el valor de retorno del thread.
De esta manera, cuando un thread hace join a otro, se bloquea hasta que el thread que hizo join termine (cuando llega al llamado al sistema Exit), y cuando termina, el thread que hizo join se desbloquea y continua su ejecucion.

### Create
El llamado al sistema Create fue relativamente facil de implementar, al recibir los parametros necesarios, la function llama por debajo al system call creat de Unix, este crea un archivo con el nombre especificado en el directorio actual y nos devuelve un identificador de archivo.

### Open
De manera similar al llamado a sistema Create, el Open fue relativamente facil de implementar, ya que una vez el openFilesTable implementado solo tenemos que usar el llamado a sistema de Unix "open" para abrir el archivo especificado y nos devuelve un identificador de archivo. Una vez abierto guardo este identificador en la tabla de archivos abiertos.

### Tabla de archivos abiertos
Para implementar la tabla de archivos abiertos tuve que usar varias implementaciones differentes, un de esas fue una version conteniendo varios campos adicionales para los sockets, pero como explicare mas abajo, al final no se logro implementar los sockets de manera functional, por lo que se tuvo que cambiar la implementacion de la tabla de archivos abiertos.
La implementacion final de la tabla de archivos usa los headers dados por el profesor en una de las clases, consiste en un int* que contiene los identificadores de archivos abiertos, y un BitMap que nos permite saber cuales identificadores estan libres y cuales no.
Ademas de eso se tiene una variable usage que permite saber cuantos threads los usan al mismo tiempo.
#### Tabla de archivos heredados por el padre
El profesor dio mucho enfasis sobre lo importante que es que cada thread no tenga tabla de archivos salvo si lo hereda del padre o si abre un archivo, lo cual apartir de este momento se crea la tabla. La tabla tambien estara compartida con sus hijos mientras que se haya hecho el fork despues de crear la tabla de archivos abiertos.

### Read & Write
Los llamados al sistema Read y Write permiten leer y escribir en algun archivo, para implementarlo se uso un switch como recomendado por el profesor, ya que podemos tener 3 subcasos mas importantes:
* entrada estandar
* salida estandar
* default

Write no puede trabajar con el ConsoleInput ya que es una llamado al sistema de salida, no de entrada, por lo que si recibe este descriptor devolvera un error.
De igual manera Read no puede trabajar con el ConsoleOutput ya que es una llamado al sistema de salida, no de entrada, por lo que si recibe este descriptor devolvera un error.

En ambos casos el descriptor default sera el que se usara mas seguido. Read lee el contenido del archivo y lo guarda en memoria, mientras que Write escribe el contenido de memoria en el archivo.

### Close
El llamado al sistema Close permite cerrar un archivo, por lo que se vuelve a usar la tabla de archivos abiertos para saber si el archivo esta abierto o no, y si lo esta, se cierra.

### Exec
El llamado al sistema Exec permite ejecutar un programa, es uno de los llamados al sistema mas imporante ya que permite ejecutar programas que no estan en el sistema operativo, de manera similar a la function fork, necesito una function auxiliar **AuxExec**, llamada por **NachOS_Exec**. 
La function crea una nuevo hilo, y se le da al nuevo hila un addrspace junto con un ejecutable que se le pasa por parametro.

### spaceID
El mi implementacion del problema use el spaceID, un int que cada thread posee, es un valor unico dentro de la simulation que permite identificar el thread de los demas, se usa por ejemplo al momento de realizar el join y el exit.

### Socket & red
Como se puede apreciar en el codigo, se intento implementar los llamados al sistema de socket, connect y bind, pero no se logro implementar de manera functional, por lo que se procedio a comentar el codigo de los llamados al sistema de socket, connect y bind.
De similar manera, como ya mencionado, se intento implementar los sockets en la tabla de archivos abiertos, pero al volverse una architectura mas compleja, se decidio cambiar la implementacion de la tabla de archivos abiertos.

### tablas de semaphoros y de sockets
Como alternativa, se dicidio implementar una tabla de semaphoros y una tabla de sockets, que permiten guardar los semaphoros y los sockets abiertos por los threads, de manera similar a la tabla de archivos abiertos, se usa un BitMap para saber cuales estan libres y cuales no.
Por cuestion de tiempo, no tuve tiempo de implementar el codigo, pero los headers ya estan implementados.