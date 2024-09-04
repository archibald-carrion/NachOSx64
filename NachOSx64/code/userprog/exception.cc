#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include "bitmap.h"
#include "addrspace.h"
#include <fcntl.h> 
#include <unistd.h>
#include <iostream>
#include "../threads/synch.h"
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <stdio.h>

#include <arpa/inet.h>  // for inet_pton
#include <sys/types.h>  // for connect 
#include <sys/socket.h>

#ifdef VM
#include "../userprog/invertedpagetables.h"  // for invertedPageTable
#include "../userprog/memoryswap.h"
#endif

#include <map>
std::map<int, bool> machineSockets;
 
#define openFilesTable currentThread->space->openFilesTable


#ifdef VM
void moveToTLB(int virtualPage) {

}




/**
 * @brief NachOS_PageFaultException is the function that handle the page fault exception
 * it receive the badVAddr from the BadVAddrReg and check if the page is already loaded in the
 * pageTable of the addrSpace of the currentThread, if so it replace the page in the tlb
 * if not it load the page from the swap memory or the disk and then replace the page in the tlb
*/
void NachOS_PageFaultException() {
   // for each PageFaultException the correct stat must be incremented
   stats->PageFaultException++;

   // read the page that provoqued the Exception from BadVAddrReg
   int badVAddr = machine->ReadRegister(BadVAddrReg);

   AddrSpace* space;
   space = currentThread->space;

   // check in the addrSpace if the page is loaded in the pageTable of currentThread->space and if so replace the tbl
   int isInAddrSpacePT = space->isLoadedInPT(badVAddr);

   // std::cout << "badVAddr: " << badVAddr << std::endl;

   int virtualAddr = space->getVirtualPage(badVAddr);
   // case were the page is already stored in the pageTable of the addrSpace
   // if (isInAddrSpacePT != -1) {
   if (isInAddrSpacePT) {
      // std::cout << "loading from pageTable" << std::endl;
      TranslationEntry* tableEntryToReplace = nullptr;

      // search the tlb for an empty entry
      for (int i = 0; i < TLBSize; i++) {
         int isEntryInvalid = !machine->tlb[i].valid;
         if(isEntryInvalid) {
            tableEntryToReplace = &(machine->tlb[i]);
            break;
         }
      }

      // if tableEntryToReplace is nullptr then no empty page were found in the tlb
      if (tableEntryToReplace == nullptr) {
         //std::cout << "no free space found, using replacement algorithm" << std::endl;
         tableEntryToReplace = invertedPageTable->getTLBEntryToReplace();
      }

      // if there were no page badVAddr in the pageTable the function would return -1
      // but the program already checked that badVAddr is loaded in pageTable
      // int virtualAddr = space->getVirtualPage(badVAddr);

      // use the load function from addrSpace to load in main memory the given page
      space->loadToTLB(tableEntryToReplace, virtualAddr);


   } else {
      bool isLoadedInSwap = false;
      // check if the segment is load in swap
      // isLoadedInSwap = memorySwap->isLoadedInSwap(badVAddr);
      isLoadedInSwap = memorySwap->isLoadedInSwap(virtualAddr);


      // load from swap
      if (isLoadedInSwap) {
         // std::cout << "loading from swap" << std::endl;
         space->loadFromSwap(virtualAddr);
      } else {
         /* if the page couldn't be loaded neither from the pageTable of the addrSpace neither from the swap memory,
         then we must load it from the disk */
         // std::cout << "loading from disk" << std::endl;
         virtualAddr = space->loadFromDisk(badVAddr);
      }

      TranslationEntry* tableEntryToReplace = nullptr;

      // search the tlb for an empty entry
      for (int i = 0; i < TLBSize; i++) {
         int isEntryInvalid = !machine->tlb[i].valid;
         if(isEntryInvalid) {
            tableEntryToReplace = &(machine->tlb[i]);
            break;
         }
      }

      // if tableEntryToReplace is nullptr then no empty page were found in the tlb
      if (tableEntryToReplace == nullptr) {
         // std::cout << "no free space found, using replacement algorithm" << std::endl;
         tableEntryToReplace = invertedPageTable->getTLBEntryToReplace();
      }

      // use the load function from addrSpace to load in main memory the given page
      // space->loadToTLB(tableEntryToReplace, badVAddr);
      space->loadToTLB(tableEntryToReplace, virtualAddr);
   }
}
#endif

void NachOS_Halt() {
	DEBUG('a', "Shutdown, initiated by user program.\n");
    currentThread->Finish();
   	interrupt->Halt();
}

void returnFromSystemCall() {
   machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
   machine->WriteRegister(PCReg, machine->ReadRegister( NextPCReg ));
   machine->WriteRegister( NextPCReg, machine->ReadRegister( NextPCReg ) + 4 );
}

void NachOS_Exit() {
   //int status = machine->ReadRegister(4);
   // currentTread->resetValue;
   currentThread->Finish();
   returnFromSystemCall();
}

/*
 *  Auxiliary function for Exec
 */
void AuxExec(void* param) {
   char* filename = (char*) param;
   OpenFile* exe = fileSystem->Open(filename);
   AddrSpace* space;

   if(exe == NULL) {
      printf("Unable to open file %s\n", filename);
      return;
   }
   
   space = new AddrSpace(exe);
   currentThread->space = space;
   
   // delete exe;

   space->InitRegisters();
   space->RestoreState();

   machine->Run();
   ASSERT(false);
}

/*
 *  System call interface: SpaceId Exec( char * )
 */
void NachOS_Exec() {		// System call 2
   Thread* newThread = new Thread("new thread");

   // int spaceID = processTable->AddProcess();
   int spaceID = 0; // not usefull right now, import is multiple thread & Fork + Exit + Joins
   newThread->spaceId = spaceID;

   newThread->Fork(AuxExec, reinterpret_cast<void*>(machine->ReadRegister( 4 )));

   currentThread->Yield();

   machine->WriteRegister(2, spaceID);    // write the given spaceID to R2

   returnFromSystemCall();
}
   
void NachOS_Join() {
   //  SpaceId spaceId = machine->ReadRegister(4);
      // TODO: add the snippet that use the semaphore
    returnFromSystemCall();
}


/*
 *  System call interface: void Create( char * )
 */
void NachOS_Create() {		// System call 4
   int fileNameAddress = machine->ReadRegister(4);

   int nameCharBuffer = 0;
   int charOnNamePos = 0;

   char* fileName =  new char[200]; // name size chosen arbitrarily
   int dynamicStringCapacity = 200;

   do {
      // adjust name buffer size if max is reached
      if (charOnNamePos == dynamicStringCapacity - 1) {
         char* fileNameBuffer = fileName;
         dynamicStringCapacity *= 2;
         fileName = new char[dynamicStringCapacity];
         memcpy(fileName, fileNameBuffer, charOnNamePos);
         delete [] fileNameBuffer;
      }

      // add char to the name
      machine->ReadMem(fileNameAddress + charOnNamePos,
         1,
         &nameCharBuffer);

      // if not end of file
      if (nameCharBuffer != 0) {
         // copy char onto name string
         fileName[charOnNamePos] = (char) nameCharBuffer;
      }

      // increase position of next char to check
      charOnNamePos++;
   } while (nameCharBuffer != 0); // ends on end of file character

   OpenFileId fileId = creat(fileName, 0777);

   if (fileId == -1) {
      machine->WriteRegister(2, -1);
   } else {
      machine->WriteRegister(2, fileId);
   }

   delete [] fileName;

   returnFromSystemCall();
}

/*
 *  System call interface: OpenFileId Open( char * )
 */
int iteratorOpen = 0;
void NachOS_Open() {
   int addr = machine->ReadRegister(4); // addr to filename
   if(addr == 0) {
      DEBUG('a', "Error: invalid address to filename: void NachOS_Open()");
   }
   char charBuffer;
   char fileName[FILENAME_MAX + 1];
   memset(fileName, '\0', FILENAME_MAX+1);

   // loop to read the filename from memory char by char   
   for(int i = 0; ; i++) {
      if (!machine->ReadMem(addr + iteratorOpen, 1, (int*)&charBuffer)) {
         DEBUG('a', "Error: (!machine->ReadMem(addr, 1, (int*)&fileName))");
      }
      fileName[iteratorOpen] = charBuffer;

      if (charBuffer == '\0') {
         break;
      }
      iteratorOpen++;
   }

   fileName[iteratorOpen] = '\0';
   // std::cout << "fileName: " << fileName << std::endl;

   int fdOpenFile = open(fileName, O_CREAT | O_RDWR, 0666);

   // if file descriptor is -1, then the file couldn't be opened
   if (fdOpenFile == -1) {
      DEBUG('a', "given file couldn't be opened: NachOS_Open()");
      machine->WriteRegister(2, -1);
   } else {

      int isOpened = openFilesTable->Open(fdOpenFile);
      if (isOpened == -1) {
         // std::cout << "could add file to openFilesTable" << std::endl;
         machine->WriteRegister(2, -1);
      } else {
         // std::cout << "added file to openFilesTable" << std::endl;
         machine->WriteRegister(2, isOpened); //Devuelve el nachosHandler
      }
   }
   iteratorOpen = 0;

   returnFromSystemCall();

}

/*
 *  System call interface: OpenFileId Write( char *, int, OpenFileId )
 */
void NachOS_Write() {		// System call 6
   int addr = machine->ReadRegister(4);
   int size = machine->ReadRegister(5);  
   int descriptor = machine->ReadRegister(6);   

   char* content = new char[size+1];

   int i = 0;

   while( i < size){
      machine->ReadMem(addr+i, 1, (int*)&content[i]);

      if (content[i] == '\n') {
         break;
      }
      i++;
   }

   // TODO: shouldn't it be i instead of ++i ?
   content[++i] = '\0';  

   switch (descriptor) {

        case  ConsoleInput: {
            machine->WriteRegister( 2, -1 );
            break;
        }

        case  ConsoleOutput: {
            printf("%s",content);
            machine->WriteRegister(2, size);
           break;
        }

        default: {
            // check if the file was already opened
            if (openFilesTable->isOpened(descriptor)) {
               int unixHandle = openFilesTable->getUnixHandle(descriptor);
               write(unixHandle, content, size);
               machine->WriteRegister(2, size);
            } else{
               // return error value if the file was not yet opened
               machine->WriteRegister(2, -1);
            }
            break;
         }
   }

   delete[] content;

   returnFromSystemCall();		// Update the PC registers
}       // NachOS_Write

/*
 *  System call interface: OpenFileId Read( char *, int, OpenFileId )
 */
void NachOS_Read() {		// System call 7
   // read the 3 parameters given by the user
   int addr = machine->ReadRegister(4);
   int size = machine->ReadRegister(5);
   int descriptor = machine->ReadRegister(6);
   char* content = new char[size];
   
   // console_semaphore->V();

   switch (descriptor) {
      case ConsoleInput: {
         
         char charBuffer = 0;
         int position = 0;
         do {
            read(1, &charBuffer, 1);
            if (charBuffer > 8) {
               content[position] = charBuffer;
            }
            position++;
         } while (charBuffer != 0 && position < size);

        if (position != size) {
            content[position] = '\n';
        }

        int readingIndex = 0;
        for (; content[readingIndex]!=0 || readingIndex<(int)strnlen(content, size) || readingIndex<size ; ++readingIndex) {
            machine->WriteMem(addr + readingIndex, 1, content[readingIndex]);
         }

         machine->WriteRegister(2, readingIndex);
         break;
      }

      case ConsoleOutput: {
         machine->WriteRegister(2, -1);
         break;
      }

      default: {
         if (openFilesTable->isOpened(descriptor)) {
            int unixHandle = openFilesTable->getUnixHandle(descriptor);
            int bytresRead = 0;
            bytresRead = read(unixHandle, content, size);
            machine->WriteRegister(2, size);
            for (int charPos = 0; charPos < bytresRead; charPos++) {
               machine->WriteMem(addr + charPos, 1, content[charPos]);
            }
            machine->WriteRegister(2, bytresRead);
         }
         else{
            machine->WriteRegister(2, -1);
         }
         break;
      }
   }
   delete[] content;
   
   //    // TODO: uncomment next line
   //    console_semaphore->V();
   returnFromSystemCall();		// Update the PC registers
}

void NachOS_Close() {
   OpenFileId fileId = machine->ReadRegister(4);
   int unixHandle = openFilesTable->Close(fileId);

   if (unixHandle == -1) {
      // report file not opened
      machine->WriteRegister(2, -1);
      returnFromSystemCall();
      return;
   }

   int result = close(unixHandle);

   if (result == 0) {
      result++;
   }
   machine->WriteRegister(2, result);
   returnFromSystemCall();
}

void ForkAux( void * p ) { // for 64 bits version
   AddrSpace *space;

   space = currentThread->space;
   space->InitRegisters();             // set the initial register values
   space->RestoreState();              // load page table register
   openFilesTable->addThread();

   machine->WriteRegister( RetAddrReg, 4 );
   machine->WriteRegister( PCReg, (long) p );
   machine->WriteRegister( NextPCReg, (long) p + 4 );

   machine->Run();                     // jump to the user progam
   ASSERT(false);
}

void NachOS_Fork() {
    Thread * newT = new Thread("child thread");

    newT->space = new AddrSpace(currentThread->space);

    newT->Fork(ForkAux, (void*)(size_t) machine->ReadRegister(4));

    returnFromSystemCall(); // This adjust the PrevPC, PC, and NextPC registers
}

void NachOS_Yield() {		// System call 10
    currentThread->Yield();
    returnFromSystemCall();  // This adjust the PrevPC, PC, and NextPC registers
}

void NachOS_SemCreate() {		// System call 11
}


void NachOS_SemDestroy() {		// System call 12
}

void NachOS_SemSignal() {		// System call 13
}

void NachOS_SemWait() {		// System call 14
}

void NachOS_LockCreate() {		// System call 15
}

void NachOS_LockDestroy() {		// System call 16
}

void NachOS_LockAcquire() {		// System call 17
}

void NachOS_LockRelease() {		// System call 18
}

void NachOS_CondCreate() {		// System call 19
}

void NachOS_CondDestroy() {		// System call 20
}

void NachOS_CondSignal() {		// System call 21
}

void NachOS_CondWait() {		// System call 22
}

void NachOS_CondBroadcast() {		// System call 23
}

void NachOS_Socket() {
   // read famility and type of socket from R4 and R5
   int family = machine->ReadRegister(4);
   int type = machine->ReadRegister(5);

   if (family == AF_INET_NachOS) {
      family = AF_INET;
   } else {
      family = AF_INET6;
   }

   if (type == SOCK_STREAM_NachOS) {
      type = SOCK_STREAM;
   } else {
      type = SOCK_DGRAM;
   }
   
   int idSocket = socket(family, type, 0);
   int fd = openFilesTable->Open(idSocket);  // use similar syntax to what we did in first sockets
   machine->WriteRegister(2, fd);

   // TODO: check if the IPv6 implementation actually works, wasn't tested yet
   if (family == AF_INET6) {
      machineSockets.insert({fd, true});
   }

   returnFromSystemCall();
}  // end of Socket syscall

// NachOS_Connect is the same function as we previously did at the beginning of the course
// The function was almost not modified
void NachOS_Connect() {
   int sockID = machine->ReadRegister(4);
   int ipAddress = machine->ReadRegister(5);
   int port = machine->ReadRegister(6);

   char* ip = new char[40];
   for (int i = 0; i < 40; ++i) {
      machine->ReadMem(ipAddress+i, 1, (int*)&ip[i]);
      if (ip[i] == '\0') {
         break;
      }
   }
    int fd = openFilesTable->getUnixHandle(sockID);
    auto it = machineSockets.find(fd);   // map iterator
    int st = 0;

    if (it == machineSockets.end()) {
        struct sockaddr_in  host4;
        struct sockaddr * ha;

        memset( (char *) &host4, 0, sizeof(host4) );
        host4.sin_family = AF_INET;
        st = inet_pton( AF_INET, ip, &host4.sin_addr );
        
        host4.sin_port = htons( port );
        ha = (sockaddr *) &host4;
        st = connect( sockID, ha, sizeof(host4) );
    } else {
        struct sockaddr_in6  host6;
        struct sockaddr * ha;

        memset( (char*) &host6, 0, sizeof(host6) );
        host6.sin6_family = AF_INET6;
        st = inet_pton( AF_INET6, ip, &host6.sin6_addr );

        host6.sin6_port = htons( port );
        ha = (struct sockaddr *) &host6;

        st = connect(sockID, ha, sizeof(host6));
    }

   // error messg if the socket couldn't connect to the objectif
    if (st == -1) {
        perror("Socket::Connect");
    }
   delete [] ip;
   returnFromSystemCall();
}

// TODO: not yet fully implemented
void NachOS_Bind() {
   int idSocket = machine->ReadRegister(4);
   int port = machine->ReadRegister(5);

   (void) idSocket;
   (void) port;

   returnFromSystemCall();
}

void NachOS_Listen() {		// System call 33
}

void NachOS_Accept() {		// System call 34
}

void NachOS_Shutdown() {	// System call 25
}


//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------

void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);

    switch ( which ) {

       case SyscallException:
          switch ( type ) {
             case SC_Halt:		// System call # 0
                NachOS_Halt();
                break;
             case SC_Exit:		// System call # 1
                NachOS_Exit();
                break;
             case SC_Exec:		// System call # 2
                NachOS_Exec();
                break;
             case SC_Join:		// System call # 3
                NachOS_Join();
                break;

             case SC_Create:		// System call # 4
                NachOS_Create();
                break;
             case SC_Open:		// System call # 5
                NachOS_Open();
                break;
             case SC_Read:		// System call # 6
                NachOS_Read();
                break;
             case SC_Write:		// System call # 7
                NachOS_Write();
                break;
             case SC_Close:		// System call # 8
                NachOS_Close();
                break;

             case SC_Fork:		// System call # 9
                NachOS_Fork();
                break;
             case SC_Yield:		// System call # 10
                NachOS_Yield();
                break;

             case SC_SemCreate:         // System call # 11
                NachOS_SemCreate();
                break;
             case SC_SemDestroy:        // System call # 12
                NachOS_SemDestroy();
                break;
             case SC_SemSignal:         // System call # 13
                NachOS_SemSignal();
                break;
             case SC_SemWait:           // System call # 14
                NachOS_SemWait();
                break;

             case SC_LckCreate:         // System call # 15
                NachOS_LockCreate();
                break;
             case SC_LckDestroy:        // System call # 16
                NachOS_LockDestroy();
                break;
             case SC_LckAcquire:         // System call # 17
                NachOS_LockAcquire();
                break;
             case SC_LckRelease:           // System call # 18
                NachOS_LockRelease();
                break;

             case SC_CondCreate:         // System call # 19
                NachOS_CondCreate();
                break;
             case SC_CondDestroy:        // System call # 20
                NachOS_CondDestroy();
                break;
             case SC_CondSignal:         // System call # 21
                NachOS_CondSignal();
                break;
             case SC_CondWait:           // System call # 22
                NachOS_CondWait();
                break;
             case SC_CondBroadcast:           // System call # 23
                NachOS_CondBroadcast();
                break;

             case SC_Socket:	// System call # 30
		NachOS_Socket();
               break;
             case SC_Connect:	// System call # 31
		NachOS_Connect();
               break;
             case SC_Bind:	// System call # 32
		NachOS_Bind();
               break;
             case SC_Listen:	// System call # 33
		NachOS_Listen();
               break;
             case SC_Accept:	// System call # 32
		NachOS_Accept();
               break;
             case SC_Shutdown:	// System call # 33
		NachOS_Shutdown();
               break;

             default:
                printf("Unexpected syscall exception %d\n", type );
                ASSERT( false );
                break;
          }
          break;

       case PageFaultException: {
         #ifdef VM
         // there is no page fault handling if not executing Virtual Memory version
         NachOS_PageFaultException();
         #endif
         break;
       }

       case ReadOnlyException:
          printf( "Read Only exception (%d)\n", which );
          ASSERT( false );
          break;

       case BusErrorException:
          printf( "Bus error exception (%d)\n", which );
          ASSERT( false );
          break;

       case AddressErrorException:
          printf( "Address error exception (%d)\n", which );
          ASSERT( false );
          break;

       case OverflowException:
          printf( "Overflow exception (%d)\n", which );
          ASSERT( false );
          break;

       case IllegalInstrException:
          printf( "Ilegal instruction exception (%d)\n", which );
          ASSERT( false );
          break;

       default:
          printf( "Unexpected exception %d\n", which );
          ASSERT( false );
          break;
    }

}
