
#ifndef OPENFILESTABLE_H
#define OPENFILESTABLE_H
#include "bitmap.h"
#include "list.h"


#define tableSize 100

class NachosOpenFilesTable {
  public:

    NachosOpenFilesTable();

   ~NachosOpenFilesTable();

    int Open( int UnixHandle );

    int Close( int NachosHandle );

    bool isOpened( int NachosHandle );

    int getUnixHandle( int NachosHandle );

    void addThread();

    int delThread();

    void Print();

    int* openFiles;                   // A vector with user opened files
    BitMap* openFilesMap;                // A bitmap to control our vector
    int usage;                         // How many threads are using this table
};

#endif 

#ifndef SOCKETABLE
#define SOCKETABLE

// class that contains all the sockets opened by the user program
class NachosSocketTable {
  public:
    NachosSocketTable();
    ~NachosSocketTable();

    int Open(int sockID, int family, int type);
    int Close(int sockID);
    bool isOpened(int sockID);
    int getFamily(int sockID);
    int getType(int sockID);
    int getUnixSocket(int sockID);
    void addThread();
    int delThread();
    void Print();

    int* openSockets;
    BitMap* openSocketsMap;
    int usage;
};
#endif //SOCKETABLE

#ifndef SEMTABLENACH
#define SEMTABLENACH

// class that contains all the semaphores opened by the user program
class NachosSemTable {
  public:
    NachosSemTable();
    ~NachosSemTable();

    int Open(int semID, int initValue);
    int Close(int semID);
    bool isOpened(int semID);
    int getSem(int semID);
    void addThread();
    int delThread();
    void Print();

    int* openSems;
    BitMap* openSemsMap;
    int usage;
};
#endif // SEMTABLENACH


// // #include "synch.h"
// // class Semaphore;
// // TODO: check that only declaring the Semaphore class without includes actually works

// // const int MAX_PROCESS = 128;
// const int MAX_OPEN_FILES = 32;


// class NachosOpenFilesTable {
//   public:
//     NachosOpenFilesTable();						// Initialize
//     ~NachosOpenFilesTable();						// De-allocate

//     int Open(  int UnixHandle);// Register the file handle
//     // family and type are used only for sockets (-1 otherwise)
//     int Close( int NachosHandle );					// Unregister the file handle
//     bool isOpened( int NachosHandle );
//     int getUnixHandle( int NachosHandle );
//     int getSockType( int NachosHandle );
//     int getSockFamily( int NachosHandle );
//     void addThread();							// If a user thread is using this table, add it
//     void delThread();							// If a user thread is using this table, delete it
//     void Print();								// Print contents

//   private:
//     int openFiles[MAX_OPEN_FILES][4];							// A vector with user opened files
//     // [Nachosid][Unixid][family(socket)][type(socket)]
//     BitMap* openFilesMap;						// A bitmap to control our vector
//     int usage;									// How many threads are using this table

// };

// #endif //OPENFILESTABLE_H


// // #ifndef NACHOS_PROCESSTABLE
// // #define NACHOS_PROCESSTABLE

// // struct processData {
// //   Semaphore* processSemaphore;
// //   int returnValue;
// //   // int spaceID;
// // };

// // class NachosProcessTable{
// //   public:

// //   NachosProcessTable();
// //   ~NachosProcessTable();

// //   int AddProcess(); // return a int spaceid
// //   void DeleteProcess(int process);

// //   void wait(int process);          // wait used in join function
// //   void signal(int process);        // signal used in exit function

// //   // processData nachosProcessTable;
// //   processData* nachosProcessTable[128];
// //   int nextFree;
// // };
// // #endif //NACHOS_PROCESSTABLE