// addrspace.h 
//  Data structures to keep track of executing user programs 
//  (address spaces).
//
//  For now, we don't keep any information about address spaces.
//  The user level CPU state is saved and restored in the thread
//  executing the user program (see thread.h).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef ADDRSPACE_H
#define ADDRSPACE_H

#include "copyright.h"
#include "filesys.h"
#include "bitmap.h"

// #include "NachosOpenFilesTable.h"

#define UserStackSize       1024    // increase this as necessary!

#define tableSize 100

class NachosOpenFilesTable {
  public:

    NachosOpenFilesTable(){
        openFilesMap = new BitMap(tableSize);
        openFiles = new int[tableSize];

        memset(openFiles, -1, tableSize);
        openFilesMap->Mark(0);
        openFilesMap->Mark(1);
        openFilesMap->Mark(2);
        openFiles[0] = 0;
        openFiles[1] = 1;
        openFiles[2] = 2;

        usage = 1;
   } 
   ~NachosOpenFilesTable(){
      delete openFilesMap;
      delete[] openFiles;
      usage = 0;
   }
    int Open( int UnixHandle ){
        int handle = 0;

        for (int openFilesIndex = 0; openFilesIndex < tableSize; openFilesIndex++) {
            if (!openFilesMap->Test(openFilesIndex)) {
                continue;
            }

            if (openFiles[openFilesIndex] == UnixHandle) {
                handle = openFilesIndex;
                break;
            }
        }
        if (handle != 0) {
            return handle;
    }
    handle = openFilesMap->Find();

    if (handle == -1) {
        return -1;
    }

    openFiles[handle] = UnixHandle;

    return handle;
   }         
    int Close( int NachosHandle ){
        if (!isOpened(NachosHandle)||NachosHandle<0||NachosHandle>tableSize) {
            return -1;
        }
        int UnixHandle = openFiles[NachosHandle];
        openFilesMap->Clear(NachosHandle);
        openFiles[NachosHandle] = -1;
        return UnixHandle;
   }
    bool isOpened( int NachosHandle ){
        if (NachosHandle < 0 || NachosHandle > tableSize) {
            return false;
        }
        return openFilesMap->Test(NachosHandle);
    }
    int getUnixHandle( int NachosHandle ){
        if (NachosHandle < 0 || NachosHandle > tableSize) {
            return -1;
        }
        return openFiles[NachosHandle];
    }
    void addThread(){
        usage++;
    }   
    int delThread(){
        usage--;
        return usage;
    }
    void Print(){
        for (int file = 0; file < tableSize; file++) {
            if (isOpened(file)) {
                printf("%i, %i\n", file, openFiles[file]);
            }
        }
    }

  // private:
        int* openFiles;                   // A vector with user opened files
        BitMap* openFilesMap;                // A bitmap to control our vector
        int usage;                         // How many threads are using this table
};

class AddrSpace {
  public:
    AddrSpace(OpenFile *executable);    // Create an address space,
                    // initializing it with the program
                    // stored in the file "executable"

    AddrSpace(AddrSpace *other);

    NachosOpenFilesTable* openFilesTable;
    OpenFile* ownExecutable;

    ~AddrSpace();           // De-allocate an address space

    void InitRegisters();       // Initialize user-level CPU registers,
                    // before jumping to user code

    void SaveState();           // Save/restore address space-specific
    void RestoreState();        // info on a context switch 

    #ifdef VM
    int isLoadedInPT(int addr);
    int getVirtualPage(int addr);
    void loadToTLB(TranslationEntry* tableEntryToReplace, int virtualAddr);
    void loadFromSwap(int virtualPage);
    int loadFromDisk(int addr);
    // int getPositionOnFile(int virtualPage);
    bool swap(int virtualPage, int physicalPage);
    #endif

  private:
    TranslationEntry *pageTable;    // Assume linear page table translation
                    // for now!
    unsigned int numPages;      // Number of pages in the virtual 
                    // address space

    void ExeNoVirtualMemoryConstructor(OpenFile *executable);
    void ExeVirtualMemoryConstructor(OpenFile *executable);
    void NoVirtualMemoryConstructor(AddrSpace * other);
    void VirtualMemoryConstructor(AddrSpace * other);



};

#endif // ADDRSPACE_H
