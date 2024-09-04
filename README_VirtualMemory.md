# Entrega Virtual Memory
## System Call NachOS_PageFaultException
Para poder realizar la memoria virtual se tuvo que agregar una nueva system call a NachOS, la cual se llama NachOS_PageFaultException. Este system call se encarga de manejar los pageFaults que se producen y consiste en 3 casos principales, los cuales son:
* Cargar desde la pageTable del AddrSpace
    * Si la pagina virtual que se necesita cargar a memoria se encuentra en la pageTable del AddrSpace, se carga a memoria y se actualiza la TLB. Es el caso mas rapido de los 3, y por eso es el primero que se verifica.
* Cargar desde la memoria Swap
    * Si la pagina virtual que se necesita cargar a memoria no se encuentra en la pageTable del AddrSpace, se verifica si se encuentra en la memoria Swap, si se encuentra se carga a memoria y se actualiza la TLB. Es el segundo caso mas lento de los 3, y por eso se verifica despues de la pageTable.
    * De manera similar a la vida real, en NachOS manejo la memoria swap como una archivo.
* Cargar desde el disco
    * Solo se usa si no se puede cargar desde la pageTable o desde la memoria Swap ya que es la manera mas lenta de cargar una pagina virtual a memoria porque se tiene que realizar calculos para saber donde se encuentra la pagina en el ejecutable del programa, como trabajamos en simulación no se nota tanto al differencia, pero en un caso de la vida real el tiempo de acceso seria similar que el de la memoria swap ya que ambos se encuentran en el disco, pero cargar desde disco tambien presenta un overhead de calculos considerable.

### PseudoCodigo
```c++
page = readPageFaultRegister

if page is loaded in pageTable of AddrSpace
    load page from pageTable to memory
    update TLB
else 
    if page is loaded in Swap
        load page from Swap to memory
    else
        load page from disk to memory
    update TLB 
return
```	

## Modificaciones a la clase AddrSpace
### Constructores de AddrSpace
La clase AddrSpace posee dos constructores, uno que recibe un puntero hacia otra instancia de la clase AddrSpace, y otro que recibe un puntero hacia la clase OpenFile (un ejecutable), al trabajar con memoria virtual tuve que modificar ambos constructores.
Para simplificar la tarea de modificar los constructores decidi dividir ambos constructores usando un "ifdef" para llamar a un "subconstructor" en caso de usar memoria virtual, y otro en caso de no usarla. Se hizo lo mismo para el segundo constructor.
De esa manera se simplifico el paso de debugear esa parte del programa ya que solo se tenia que modificar un constructor a la vez.

La mayor differencia entre ambas versiones (ejecutable y "other" versión con VM y sin VM) consiste en el constructor para memoria virtual no verifica la cantidad de paginas necesarias ni lo copia directo a memoria ya que "poco a poco" se iran cargando a memoria las paginas necesarias. En el otro caso la cantidad de paginas necesarias se calcula y se copia todo a memoria, si no se puede copiar todo a memoria se mata el proceso.

### Nuevas funciones de addrSpace
Para realizar la memoria virtual, ademas de los constructores, se agregaroon multiples funciones a la clase AddrSpace, las cuales son:
* int isLoadedInPT(int addr);
    * isLoadedInPT verificar si el addr dado existe en la page table de la instancia de AddrSpace, si existe retorna 1, si no existe retorna 0.
    * se usa como "conditional" para saber si se debe cargar a memoria una pagina desde el pageTable del addrSpace o no.
    * existe una función similar que permite realizar lo mismo pero para la memoria Swap, se hablara de eso mas adelante.
* int getVirtualPage(int addr);
    * getVirtualPage retorna el numero de pagina virtual a la que pertenece el addr dado.
    * se usa para saber a que pagina virtual se debe cargar a memoria.
* void loadToTLB(TranslationEntry* tableEntryToReplace, int virtualAddr);
    * loadToTLB se usa despues de los tres casos (cargar desde pageTable, Swap o disco), permite cargar a la TLB la pagina virtual dada ademas de actualizar la pageTable con la entrada de la TLB que se tiene que quitar. Esa entrada solo se guarda en la pageTable si no esta vacia.
* void loadFromSwap(int virtualPage);
    * loadFromSwap carga a memoria la pagina virtual dada desde la memoria Swap.
    * se usa cuando se necesita cargar una pagina virtual desde la memoria Swap lo cual se usa cuando se occupa mas memoria que lo que se tiene disponible.
* int loadFromDisk(int addr);
    * loadFromDisk carga a memoria la pagina virtual dada desde el disco.
    * los primeros X pageFault que se producen se cargan desde el disco, despues usualemente se cargan desde la pageTable de addrSpace o desde la memoria Swap.
* bool swap(int virtualPage, int physicalPage);
    * se usa cuando se lleno la memoria disponible, la inverted page table busca campo en su tabla de pagina fisica y si no encuentra campo, llama a la función swap de addrspace.

## Modificaciones al archivo system.cc en la carpeta thread
Para poder tener acceso en exception.cc, invertedpagestables.h, addrspace.cc y memoryswap.h a las instancias **invertedPageTable** y **memorySwap** se tuvo que modificar el archivo system.cc en la carpeta thread de manera crear, inicializar y destruir esas instancias cuando se compila el codigo con la bandera VM.

## Uso de la instancia TLB en vez de la pageTable
Como se ve en el archivo translate.cc alredor de la linea 230:
```c++
    ASSERT(tlb == NULL || pageTable == NULL);	
    ASSERT(tlb != NULL || pageTable != NULL);	
```
Solo se puede tener o la TLB o la pageTable, no ambas. Por lo que tuve que modificar la function RestoreState de la clase AddrSpace para que deje de actualizar la pageTable:
```c++
void AddrSpace::RestoreState() {
    #ifndef VM
    machine->pageTable = pageTable;
    machine->pageTableSize = numPages;
    #else
    /**
     * alternative for VM
     */
    #endif
}
```

## Nuevas clases implementadas
Para poder realizar de manera optima la memoria virtual se tuvo que crear 2 nuevas clases, la invertedPageTable y la memorySwap. Ambas clases se encuentran en la carpeta userprog mas que todo por consistencia con el resto del codigo que se escribio para el proyectio anterior tal como la clase AddrSpace y el archivo exception.cc, pero muy bien podrian estar en la carpeta vm, solo se occuparia cambiar ligeralmente el Makefile y los includes a esos archivos.

## InvertedPageTable
La clase invertedPageTable se encarga de manejar la tabla de paginas invertida, la cual se usa para saber que paginas fisicas estan ocupadas y cuales no. La clase invertedPageTable se encarga de inicializar, y buscar campos en la tabla de paginas invertida. La clase invertedPageTable se encuentra en el archivo invertedpagestable.h y se encuentra en la carpeta userprog. En case de no usar memoria virtual, la clase invertedPageTable no se usa. Si no se encuentra un campo en la tabla de paginas invertida, se llama a la funcion swap de la clase AddrSpace como ya se menciono anteriormente.

### Algortimo de reemplazo
En mi implementación de la memoria virtual decidi implementar tres algoritmos de reemplazo, los cuales son:
* FIFO
    * usa una queue para saber cual pagina se cargo primero a memoria
* Second Chance
    * usa un bit de referencia para saber si se uso o no la pagina
* Pagina con menos usos
    * reemplaza la pagina que se uso menos veces

De esa manera dependiendo del valor dado en el constructor de la clase invertedPageTable se usa un algoritmo de reemplazo u otro. 
Para eso se puede cambiar el valor replacementAlgorithm en la function Initialize del archivo system.cc en la carpeta thread:
```c++
#ifdef VM
    int replacementAlgorithm = 1;
    invertedPageTable = new invertedpagetable(replacementAlgorithm);
    // IMPORTANT: the numerical values of the algorithms are: 0 for FIFO, 1 for Second Chance, 2 for Least Used
    memorySwap = new swap();
#endif
```

Cada funcion que usa de alguna manera un algoritmo de reemplazo tiene por dentro un switch case que se encarga de realizar la buena funcion dependiendo del valor de replacementAlgorithm.

### Atributos de la clase invertedPageTable
* std::vector<int> uses
    * uses se usa para el algoritmo de reemplazo Least Used, cada vez que se usa una pagina se aumenta en 1 el valor de uses de esa pagina. Si se usa Second Chance o FIFO, uses no se usa.
* std::vector<int> virtualPageTable
    * virtualPageTable se usa para realizar un mapeo entre la pagina fisica, la posicion, y la pagina virtual, el valor de la posicion.
* std::list<int> fifo_queue
    * fifo_queue se usa para el algoritmo de reemplazo FIFO
* int pageCounter
    * pageCounter se usa para saber cuantas paginas fisicas se han usado, se inicializa en 0 y se aumenta en 1 cada vez que se usa una pagina fisica.
* std::list<std::pair<int, bool>> second_chance_queue
    * second_chance_queue se usa para el algoritmo de reemplazo Second Chance, se usa un bit de referencia para saber si se uso o no la pagina fisica, si se uso se pone en 1, si no se uso se pone en 0.
* BitMap* physicalPagesBitmap
    * physicalPagesBitmap se usa para saber que paginas fisicas estan ocupadas y cuales no, se inicializa con una cantidad NumPhysPages_IPT de campos y se marca un bit en 1 cada vez que se usa una pagina fisica.
    * permite saber si ya no hay mas paginas fisicas disponibles y si por lo tanto se tiene que llamar la funcion swap de la clase AddrSpace.
* int replacementAlgorithm
    * replacementAlgorithm se usa para saber que algoritmo de reemplazo se tiene que usar, por defecto es 1, el cual corresponde a Second Chance, 0 corresponde a FIFO y 2 corresponde a Least Used.
* int NumPhysPages_IPT
    * NumPhysPages_IPT se usa para saber cuantas paginas fisicas se tienen disponibles, por defecto es 32, pero se puede cambiar en el archivo invertedpagetables.cc si queremos probar el codigo con 64 paginas fisicas o cualquier otro valor.

### Funciones de la clase invertedPageTable
* invertedpagetable(int algorithmToUse = 1) 
    * constructor de la clase invertedPageTable, se encarga de inicializar los atributos de la clase invertedPageTable.
* ~invertedpagetable()
    * destructor de la clase invertedPageTable, se encarga de liberar la memoria ocupada por los atributos de la clase invertedPageTable.
* TranslationEntry* getTLBEntryToReplace()
    * getTLBEntryToReplace se encarga de buscar una entrada en la TLB que se pueda reemplazar, se usa cuando se tiene que cargar una pagina virtual a la TLB.
* int getLeastUsedPhysicalPage()
    * getLeastUsedPhysicalPage se encarga de buscar la pagina fisica que se uso menos veces, se usa cuando se tiene que reemplazar una pagina fisica usando el algoritmo de reemplazo Least Used.
* void updatePage(int physicalPage, bool resetUses = true)
    * updatePage se encarga de actualizar la pagina fisica dada, para el algoritmo de reemplazo Second Chance, se usa para poner el bit de referencia en 1. Para el algortimo de reemplazo Least Used, se usa para aumentar en 1 el valor de uses de la pagina fisica dada. Para el algoritmo de reemplazo FIFO, se usa para agregar la pagina fisica dada a la queue fifo_queue.
*  int getPhysicalPage(int virtualPage)
    * getPhysicalPage se encarga de buscar la pagina fisica que se usa para la pagina virtual dada, se usa cuando se tiene que cargar una pagina virtual a memoria. Si no se encuentra la pagina virtual dada en la tabla de paginas invertida, se llama a la funcion swap de la clase AddrSpace para "hacer campo".
* int getEntryToReplace()
    * getEntryToReplace se encarga de buscar una entrada que se pueda reemplazar, para eso se usa el algoritmo de reemplazo dado por replacementAlgorithm.
* void RestorePages()
    * RestorePages se encarga de restaurar las paginas fisicas que se usaron por el proceso actual, se usa en cada ciclo cuando se llama a la funcion RestoreState de la clase AddrSpace.

## Swap
La clase swap se encarga de manejar la memoria Swap, la cual se usa para guardar las paginas virtuales que no se pueden cargar a memoria. La clase swap se encuentra en el archivo memoryswap.h que se encuentra en la carpeta userprog. En case de no usar memoria virtual, la clase swap no se usa.

### Atributos de la clase swap
* const int SWAP_PAGES_QUANTITY = 2*NumPhysPages
    * SWAP_PAGES_QUANTITY se usa para saber cuantas paginas virtuales se pueden guardar en la memoria Swap, por defecto es 64, pero se puede cambiar en el archivo memoryswap.h si queremos probar el codigo con cualquier otro valor.
* const std::string SWAP_FILE_NAME = "SWAP"
    * SWAP_FILE_NAME se usa para saber el nombre del archivo que se usa para guardar las paginas virtuales en la memoria Swap.
* OpenFile* swapSegment
    * swapSegment es un puntero hacia el archivo que se usa para guardar las paginas virtuales en la memoria Swap.
* BitMap* swapBitMap
    * swapBitMap se usa para saber cuales paginas estan ocupadas en la memoria Swap, y cuales no.
* std::vector<int> virtualPages
    * virtualPages se usa para saber que pagina virtual se encuentra en que pagina de la memoria Swap, se usa para poder cargar una pagina virtual desde la memoria Swap a memoria.

### Funciones de la clase swap
* swap()
    * constructor de la clase swap, se encarga de inicializar los atributos de la clase swap.
* ~swap()
    * destructor de la clase swap, se encarga de liberar la memoria ocupada por los atributos de la clase swap.
* bool isLoadedInSwap(int addr)
    * isLoadedInSwap se encarga de verificar si la pagina virtual dada se encuentra en la memoria Swap, si se encuentra retorna true, si no se encuentra retorna false.
* bool readSwapMemory(int physicalPage, int virtualPage)
    * readSwapMemory se encarga de cargar la pagina virtual dada desde la memoria Swap a memoria, si se puede cargar retorna true, si no se puede cargar retorna false.
* bool writeSwapMemory(int physicalPage, int virtualPage)
    * writeSwapMemory se encarga de guardar la pagina virtual dada desde memoria a la memoria Swap, si se puede guardar retorna true, si no se puede guardar retorna false.

## Test que funcionaron
### halt
El test de halt sirve correctamente, tiene una estadistica de PageFaultException de 4, lo cual es un numero esperado.
Ademas de eso vemos que el programa termina de ejecutarse correctamente.

![screenshot de halt](./test_screenshots/halt_execution.png)

### copy
El test de copy funciona correctamente, consiste en leer un archivo y copiarlo a otro archivo.

![screenshot de copy](./test_screenshots/copy.png)

### matmult5
El test de matmult5 funciona correctamente, tiene una estadistica de PageFaultException de alredor de 1500, lo cual es un numero esperado. 
El programa termina de ejecutarse correctamente pero por alguna razon occure un error de "double free or corruption (out)" despues de que el programa termina de ejecutarse durante la etapa de cleanup.

![screenshot de matmult5](./test_screenshots/matmult5.png)

## Test que parcialmente funcionaron
### matmult20
El test de matmult20 no funciona correctamente, logra a cargar al disco varias paginas, despues de eso occure multiples centendas (tal vez miles) de PageFaultException y el programa logra cargarlos correctamente desde la pageTable, pero despues de eso por alguna razon el programa se cae despues de fallar un Assert en la linea 182 del archivo sysdep.cc.
![screenshot de matmult20](./test_screenshots/matmult20.png)

### matmult
De manera similar a matmult20, el matmult no sirve correctamente, logra cargar varias paginas pero se cae durante la ejecución.

### shell
El programa shell funciona parcialmente, permite leer la entrada del usuario correctamente, pero despues de eso se cae.
