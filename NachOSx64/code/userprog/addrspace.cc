// addrspace.cc 
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -N -T 0 option 
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "addrspace.h"
#include "../bin/noff.h"
#include <iostream>
// #include "NachosOpenFilesTable.h"


#include "../userprog/invertedpagetables.h"  // for invertedPageTable
#include "../userprog/memoryswap.h"          // for memorySwap


//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the 
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

static void 
SwapHeader (NoffHeader *noffH)
{
	noffH->noffMagic = WordToHost(noffH->noffMagic);
	noffH->code.size = WordToHost(noffH->code.size);
	noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
	noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
	noffH->initData.size = WordToHost(noffH->initData.size);
	noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
	noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
	noffH->uninitData.size = WordToHost(noffH->uninitData.size);
	noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
	noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	Create an address space to run a user program.
//	Load the program from a file "executable", and set everything
//	up so that we can start executing user instructions.
//
//	Assumes that the object code file is in NOFF format.
//
//	First, set up the translation from program memory to physical 
//	memory.  For now, this is really simple (1:1), since we are
//	only uniprogramming, and we have a single unsegmented page table
//
//	"executable" is the file containing the object code to load into memory
//----------------------------------------------------------------------

AddrSpace::AddrSpace(OpenFile *executable)
{   
    #ifndef VM
    ExeNoVirtualMemoryConstructor(executable);
    #else
    ExeVirtualMemoryConstructor(executable);
    #endif
}


AddrSpace::AddrSpace(AddrSpace *other) {
    #ifndef VM
    NoVirtualMemoryConstructor(other);
    #else
    VirtualMemoryConstructor(other);
    #endif
}


//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Dealloate an address space.  Nothing for now!
//----------------------------------------------------------------------

AddrSpace::~AddrSpace()
{
    // clear used pages
    for (unsigned int page = 0; page < this->numPages; page++) {
        MiMapa->Clear(this->pageTable[page].physicalPage);
    }

    delete pageTable;

    // if I was the last thread using it
    if (this->openFilesTable->delThread() == 0) {
        // destroy openFilesTable
        delete this->openFilesTable;
    }
}

//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------

void
AddrSpace::InitRegisters()
{
    int i;

    for (i = 0; i < NumTotalRegs; i++)
	machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of "Start"
    machine->WriteRegister(PCReg, 0);	

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    machine->WriteRegister(NextPCReg, 4);

   // Set the stack register to the end of the address space, where we
   // allocated the stack; but subtract off a bit, to make sure we don't
   // accidentally reference off the end!
    machine->WriteRegister(StackReg, numPages * PageSize - 16);
    DEBUG('a', "Initializing stack register to %d\n", numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, nothing!
//----------------------------------------------------------------------

void AddrSpace::SaveState() {
    //std::cout << "saving state" << std::endl;
}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState() {
    #ifndef VM
    // this part of the program crash when executed without the ifndef in the 
    machine->pageTable = pageTable;
    machine->pageTableSize = numPages;
    #else
    // invertedPageTable->RestorePages();
    //std::cout << "restoring state" << std::endl;

    // the invertedpagetables must restore the pages
    #endif
}


/**
 * 
*/
void AddrSpace::ExeNoVirtualMemoryConstructor(OpenFile *executable) {

    this->openFilesTable = new NachosOpenFilesTable();
    this->ownExecutable = executable;

    NoffHeader noffH;
    unsigned int i, size;

    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && 
		(WordToHost(noffH.noffMagic) == NOFFMAGIC))
    	SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

    // how big is address space?
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size 
			+ UserStackSize;	// we need to increase the size
						// to leave room for the stack
    numPages = divRoundUp(size, PageSize);

    // find if there are enough space to fit program
    if (MiMapa->NumClear() < (int) numPages) {
        // handle lack of space
        return;
    }

    unsigned int sizeToCopy = noffH.code.size + noffH.initData.size + noffH.uninitData.size;
    sizeToCopy = divRoundUp(size, PageSize);

    size = numPages * PageSize;

    ASSERT(numPages <= NumPhysPages);		// check we're not trying
						// to run anything too big --
						// at least until we have
						// virtual memory

    DEBUG('a', "Initializing address space, num pages %d, size %d\n", 
					numPages, size);

    int pageLocation = 0;

    // first, set up the translation 
    pageTable = new TranslationEntry[numPages];
    for (i = 0; i < numPages; i++) {
        // find empty page
        // does not matter if pages are not contiguous
        // expected for address space to handle correctly by use of virtual address
        int newLocation = MiMapa->Find();
        // if page invalid
        if (newLocation == -1) {
            // for all pages marked
            int memLeft = MiMapa->NumClear();
            for (unsigned int pageToErase = 0;
                pageToErase < i;
                pageToErase++) {
                // clear them
                MiMapa->Clear(this->pageTable[pageToErase].physicalPage); 
            }
            // report error
            fprintf(stderr, "Could not allocate necessary memory, terminating thread\n"
            "Amount of memory used: %i out of %i\n", memLeft, NumPhysPages);
            // terminate thread
            currentThread->Finish();
        }

        pageLocation = newLocation;

        pageTable[i].virtualPage = i;	// for now, virtual page # = phys page #
        pageTable[i].physicalPage = pageLocation; // the location is the position within the bitmap
        pageTable[i].valid = true;
        pageTable[i].use = false;
        pageTable[i].dirty = false;
        pageTable[i].readOnly = false;  // if the code segment was entirely on 
                        // a separate page, we could set its 
                        // pages to be read-only
    }

    // for all pages
    for (unsigned int page = 0; page < sizeToCopy ; page++) {
        // find the page location
        int newPageLocation = this->pageTable[page].physicalPage;
        int positionOnFile = noffH.code.inFileAddr // begin of file location
            + (page // position of page after file start
            * PageSize); // with a page size displacement

        DEBUG('a', "Copying page at page %d,"
            " and file location 0x%x\n", 
			newPageLocation, positionOnFile);

        int pageCopySize = PageSize;

        // if last page
        if (page == sizeToCopy - 1) {
            // copy just the bytes in last page that are within file
            pageCopySize =
                (noffH.code.size + noffH.initData.size + noffH.uninitData.size)
                % sizeToCopy; 
        }

        // copy entire page into memory
        executable->ReadAt(
            &(machine->mainMemory[PageSize * newPageLocation]),  // at location of page within memory
            pageCopySize, // with the size of a page
            positionOnFile);
    }

}

/**
 * 
*/
void AddrSpace::ExeVirtualMemoryConstructor(OpenFile *executable) {
    this->openFilesTable = new NachosOpenFilesTable();
    this->ownExecutable = executable;

    NoffHeader noffH;
    unsigned int i, size;

    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && 
		(WordToHost(noffH.noffMagic) == NOFFMAGIC))
    	SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

    // how big is address space?
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size 
			+ UserStackSize;	// we need to increase the size
						// to leave room for the stack
    numPages = divRoundUp(size, PageSize);
    //std::cout << "numPages of addrSpace: " << numPages << std::endl;

    // // find if there are enough space to fit program
    // if (MiMapa->NumClear() < (int) numPages) {
    // // handle lack of space
    // return;
    // }
    // unsigned int sizeToCopy = noffH.code.size + noffH.initData.size + noffH.uninitData.size;
    // sizeToCopy = divRoundUp(size, PageSize);

    size = numPages * PageSize;
    DEBUG('a', "Initializing address space, num pages %d, size %d\n", 
					numPages, size);

    int pageLocation = -1; // TODO: change to -1 ?

    // translation 
    pageTable = new TranslationEntry[numPages];
    for (i = 0; i < numPages; i++) {
        pageTable[i].physicalPage = pageLocation;
        pageTable[i].virtualPage = pageLocation;
        pageTable[i].valid = false;
        pageTable[i].use = false;
        pageTable[i].dirty = false;
        pageTable[i].readOnly = false;
    }
}

/**
 * 
*/
void AddrSpace::NoVirtualMemoryConstructor(AddrSpace * other) {
    this->numPages = other->numPages;
    this->openFilesTable = other->openFilesTable;
    this->ownExecutable = other->ownExecutable;


    unsigned int pageLocation = 0;
    // size shared is the all sectors minus the stack
    unsigned int sharedMemory = this->numPages
        - divRoundUp(UserStackSize, PageSize);

    // first, set up the translation 
    pageTable = new TranslationEntry[this->numPages];
    unsigned int page = 0;

    // set as shared the code and data segments
    for (; page < sharedMemory; page++) {
        // set physical location as same for the father
        pageLocation = other->pageTable[page].physicalPage;

        pageTable[page].virtualPage = page;	// for now, virtual page # = phys page #
        pageTable[page].physicalPage = pageLocation; // the location is the position within the bitmap
        pageTable[page].valid = true;
        pageTable[page].use = false;
        pageTable[page].dirty = false;
        pageTable[page].readOnly = false;  // if the code segment was entirely on 
                        // a separate page, we could set its 
                        // pages to be read-only
    }

    // allocate new space for the stack
    for (; page < this->numPages; page++) {
        // find empty page
        // does not matter if pages are not contiguous
        // expected for address space to handle correctly by use of virtual address
        int newLocation = MiMapa->Find();

        // if page invalid
        if (newLocation == -1) {
            int memLeft = MiMapa->NumClear();
            // for all pages marked
            for (unsigned int pageToErase = sharedMemory;
                pageToErase < page;
                pageToErase++) {
                // clear them
                MiMapa->Clear(this->pageTable[pageToErase].physicalPage); 
            }

            fprintf(stderr, "Could not allocate necessary memory, terminating thread\n"
            "Amount of memory used: %i out of %i\n", memLeft, NumPhysPages);
            // terminate thread
            currentThread->Finish();
        }

        pageLocation = newLocation;

        pageTable[page].virtualPage = page;	// for now, virtual page # = phys page #
        pageTable[page].physicalPage = pageLocation; // the location is the position within the bitmap
        pageTable[page].valid = true;
        pageTable[page].use = false;
        pageTable[page].dirty = false;
        pageTable[page].readOnly = false;  // if the code segment was entirely on 
                        // a separate page, we could set its 
                        // pages to be read-only
    }

}

/***
 * 
*/
void AddrSpace::VirtualMemoryConstructor(AddrSpace * other) {
    this->numPages = other->numPages;
    this->openFilesTable = other->openFilesTable;
    this->ownExecutable = other->ownExecutable;


    unsigned int pageLocation = 0;
    // size shared is the all sectors minus the stack
    unsigned int sharedMemory = this->numPages
        - divRoundUp(UserStackSize, PageSize);

    // first, set up the translation 
    pageTable = new TranslationEntry[this->numPages];
    unsigned int page = 0;

    // set as shared the code and data segments
    for (; page < sharedMemory; page++) {
        // set physical location as same for the father
        pageLocation = other->pageTable[page].physicalPage;

        pageTable[page].virtualPage = other->pageTable[page].virtualPage;	// for now, virtual page # = phys page #
        pageTable[page].physicalPage = pageLocation; // the location is the position within the bitmap
        pageTable[page].valid = true;
        pageTable[page].use = false;
        pageTable[page].dirty = false;
        pageTable[page].readOnly = false;  // if the code segment was entirely on 
                        // a separate page, we could set its 
                        // pages to be read-only
    }

    // allocate new space for the stack
    for (; page < this->numPages; page++) {
        pageTable[page].physicalPage = -1;
        pageTable[page].virtualPage = -1;
        pageTable[page].valid = false;
        pageTable[page].use = false;
        pageTable[page].dirty = false;
        pageTable[page].readOnly = false;  // if the code segment was entirely on 
                        // a separate page, we could set its 
                        // pages to be read-only
    }
}

/**
 * @brief isLoadedInPT returns if the given address is loaded in the page table of the address space
*/
#ifdef VM
int AddrSpace::isLoadedInPT(int addr) {
    int page = addr / PageSize;
    // return this->pageTable[page].valid;
    // std::cout << "given addrs as a virtualPage of " << pageTable[page].virtualPage << std::endl;
    return pageTable[page].virtualPage != -1;
}
#endif

/**
 * @brief getPhysicalPage returns the physical page of a the given address
*/
#ifdef VM
int AddrSpace::getVirtualPage(int addr) {
    int page = addr / PageSize;
    int virtualPage = 0;

    if(pageTable[page].virtualPage != -1 ) {
        virtualPage = pageTable[page].virtualPage;
        // std::cout << "virtual page is not -1" << std::endl;
    } else {
        virtualPage = page;
    }
    return virtualPage;
}
#endif

/**
 * @brief loadToTLB loads the given virtual address to the TLB
*/
#ifdef VM
void AddrSpace::loadToTLB(TranslationEntry* tableEntryToReplace, int virtualAddr) {

    // store the page to replace in the page table
    if(tableEntryToReplace->valid) {
        //std::cout << "storing page to replace" << std::endl;
        int indexPage = tableEntryToReplace->virtualPage;
        pageTable[indexPage].use = tableEntryToReplace->use;
        pageTable[indexPage].dirty = tableEntryToReplace->use;
    }

    // load the new page to the TLB
    pageTable[virtualAddr].valid = true;
    // tableEntryToReplace->virtualPage = this->pageTable[virtualAddr].virtualPage;
    // tableEntryToReplace->physicalPage = this->pageTable[virtualAddr].physicalPage;
    // tableEntryToReplace->valid = this->pageTable[virtualAddr].valid;
    tableEntryToReplace->virtualPage = this->pageTable[virtualAddr].virtualPage;
    tableEntryToReplace->physicalPage = this->pageTable[virtualAddr].physicalPage;
    tableEntryToReplace->valid = this->pageTable[virtualAddr].valid;
    // content below doesn't need to be copied because exact same values won't be used in TLB
    // tableEntryToReplace->use = this->pageTable[virtualAddr].use;
    // tableEntryToReplace->dirty = this->pageTable[virtualAddr].dirty;
    // tableEntryToReplace->readOnly = this->pageTable[virtualAddr].readOnly;

    // tell the inverted page table that the page is being used
    invertedPageTable->updatePage(tableEntryToReplace->physicalPage);
}
#endif

/**
 * @brief loadFromSwap loads the given virtual address from the swap segment
 * @param virtualPage is the virtual page to be loaded
*/
#ifdef VM
void AddrSpace::loadFromSwap(int virtualPage) {
    // call sub-routine in invertedPageTable
    int physicalPage = invertedPageTable->getPhysicalPage(virtualPage);

    // load the page from swap
    bool successfulReadFromSwap = false;
    successfulReadFromSwap = memorySwap->readSwapMemory(physicalPage, virtualPage);

    // check if the read was successful
    if (!successfulReadFromSwap) {
        // TODO: add error message ?
        printf("Error reading from swap\n");
        // clean the page table from inverted page table
        // wkill the process
        // currentThread->Finish();
        return;
    }

    // do only if the read to the swap was successful
    pageTable[virtualPage].physicalPage = physicalPage;
    pageTable[virtualPage].virtualPage = virtualPage;
    pageTable[virtualPage].valid = true;
    pageTable[virtualPage].dirty = false;
}
#endif

/**
 * @brief loadFromDisk loads the given virtual address from the executable
 * @param addr is the virtual address to be loaded
*/
#ifdef VM
int AddrSpace::loadFromDisk(int addr) {
    int virtualPage = addr / PageSize;
    // get the physical page from the inverted page table same way as in loadFromSwap
    int physicalPage = invertedPageTable->getPhysicalPage(virtualPage);

    // TODO: check error ?
    if(physicalPage == -1) {
        std::cout << "killing process" << std::endl;
        // kill the process
    }

    // std::cout << "physicalPage of the wanted page: " << physicalPage << std::endl;

    pageTable[virtualPage].physicalPage = physicalPage;
    pageTable[virtualPage].virtualPage = virtualPage;

    // load the page from disk
    NoffHeader noffH;
    this->ownExecutable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    
    // same code than in ExeNoVirtualMemoryConstructor
    
    // std::cout << "in loadFromDisk function" << std::endl;


    if ((noffH.noffMagic != NOFFMAGIC) && 
		(WordToHost(noffH.noffMagic) == NOFFMAGIC))
    	SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

    // TODO: check if this is correct
    // FIXME: modify for sub-cases like last page and such ?

    int destination = pageTable[virtualPage].physicalPage * PageSize;
    int positionOnFile = noffH.code.inFileAddr // begin of file location
        + (virtualPage // position of page after file start
        * PageSize); // with a page size multiplayer


    int isStack = (addr >= noffH.initData.size + noffH.uninitData.size + noffH.code.inFileAddr + noffH.code.size);
    if(isStack) {
        // std::cout << "stack" << std::endl;
        return virtualPage;
    }

    int size = PageSize;
    int sizeExecutable = noffH.code.size + noffH.initData.size + noffH.uninitData.size;
    int isLastPage = virtualPage == (int)(this->numPages-1) && sizeExecutable%size !=0;
    if(isLastPage) {
        // std::cout << "isLastPage" << std::endl;
        // size should be a variable depending on the case
        size = sizeExecutable%size;
    }

    // destination is were the page will be loaded

    ownExecutable->ReadAt(
        &(machine->mainMemory[destination]),  // at location of page within memory
        size, // with the size of a page
        positionOnFile);

    return virtualPage;
}
#endif

/**
 * 
*/
#ifdef VM
bool AddrSpace::swap(int virtualPage, int physicalPage) {
    // std::cout << "in swap function" << std::endl;
    bool isSwaped = false;
    // reset the given page of the pageTable
    pageTable[virtualPage].physicalPage = -1;   // reset the physical page
    pageTable[virtualPage].virtualPage = -1;    // reset the virtual page
    pageTable[virtualPage].valid = false;       // invalidate the page

    for(int i = 0; i < TLBSize; ++i) {
        bool isVirtualPage = machine->tlb[i].virtualPage == virtualPage;
        if(isVirtualPage) {
            // reset the tlb entry i
            machine->tlb[i].virtualPage = -1;   // reset the virtual page
            machine->tlb[i].physicalPage = -1;  // reset the physical page
            machine->tlb[i].valid = false;      // invalidate the page
            // TODO: break the loop ?
            // only 4 iterations max, if works like this do not change
        }
    }

    isSwaped = memorySwap->writeSwapMemory(physicalPage, virtualPage);
    return isSwaped;
}
#endif