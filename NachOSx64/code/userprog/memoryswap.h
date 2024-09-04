#include "bitmap.h"

#include <vector>       // for storing 
#include <iostream>     // for debugging
#include <string>       // for std::string

class swap {
 public:
    /**
     * @param pagesQuantity is used to store the quantity of pages
    */
    const int SWAP_PAGES_QUANTITY = 2*NumPhysPages;
    // depending on the test we will have to change the value of SWAP_PAGES_QUANTITY

    /**
     * @param swapFileName is used to store the name of the swap file
    */
    const std::string SWAP_FILE_NAME = "SWAP";

    /**
     * @param swapSegment is used to store the swap segment, it's an OpenFile*
    */
    OpenFile* swapSegment;

    /**
     * @param swapBitMap is used to know wich pages are free in the swap segment
    */
    BitMap* swapBitMap;

    /**
     * @param victualPages
    */
   std::vector<int> virtualPages;

    // TODO: implement other vector to facilitate access, like virtualPage or addrSpace ?

    /**
     * @brief swap class constructor
    */
    swap() {
        // create the swap segment using the fileSystem
        fileSystem->Create("SWAP", SWAP_PAGES_QUANTITY*PageSize);
        swapSegment = fileSystem->Open("SWAP");

        // create the swapBitMap
        swapBitMap = new BitMap(SWAP_PAGES_QUANTITY);
        virtualPages = std::vector<int>(SWAP_PAGES_QUANTITY);
        // when i try to read from the virtualPages when it is empty it return that the value is loaded in swao
        // bug probably comes from the fact that the vector set initial value to "0"  ?
        // will initialize values as -1 just to be sure
        for(int i = 0; i < SWAP_PAGES_QUANTITY; ++i) {
            virtualPages[i] = -1;
        }


    }

    ~swap() {
        delete swapBitMap;
    }

    bool isLoadedInSwap(int addr) {
        bool isLoaded = false;

        for(int i =0; i < SWAP_PAGES_QUANTITY; ++i) {
            if(this->virtualPages[i] == addr) {
                isLoaded = true;
                break;
            }
        }
        return isLoaded;
    }

    bool readSwapMemory(int physicalPage, int virtualPage) {
        bool readsuccesful = false;
        // need to translate from physicalPage to address using the reverse of technique used in addrSpace
        int addr = physicalPage*PageSize;

        int fileIndex = 0;
        for(fileIndex = 0; fileIndex < SWAP_PAGES_QUANTITY; ++fileIndex) {
            if(this->virtualPages[fileIndex] == virtualPage) {
                readsuccesful = true;
                break;
            }
        }

        if(readsuccesful == false) {
            // std::cout << "Page not found in swap" << std::endl;
            return readsuccesful;
        }

        virtualPages[fileIndex] = 0;
        swapBitMap->Clear(fileIndex);

        // multiply by PageSize to get the acutal address on the swap file
        fileIndex = fileIndex*PageSize;
        
        swapSegment->ReadAt(&machine->mainMemory[addr],    // were to write to
                            PageSize,                       // how many bytes to write
                            fileIndex);             // were to read from

        return readsuccesful;
    }

    /**
     * @brief writeSwapMemory is used to write a page to the swap file
     * @param physicalPage is the physical page to write
     * @param virtualPage is the virtual page to write
    */
    bool writeSwapMemory(int physicalPage, int virtualPage) {
        bool writesuccesful = false;
        // need to translate from physicalPage to address using the reverse of technique used in addrSpace
        int addr = physicalPage*PageSize;
        int availablePosition = swapBitMap->Find();

        // if no more free space
        if(availablePosition == -1) {
            // std::cout << "No more free space in swap" << std::endl;
            writesuccesful = false;
            return writesuccesful;
        }
        
        virtualPages[availablePosition] = virtualPage;

        // multiply by PageSize to get the acutal address on the swap file
        availablePosition = availablePosition*PageSize;

        // write the page to the swap file --> using writeAt?
        swapSegment->WriteAt(&machine->mainMemory[addr],    // were to read from
                            PageSize,                       // how many bytes to read
                            availablePosition);             // were to write to

        writesuccesful = true;
        return writesuccesful;
    }

    

};
