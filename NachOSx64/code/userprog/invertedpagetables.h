#include "bitmap.h"
#include <vector>       // for internal data structures
#include <iostream>     // for debugging
#include <list>         // std::list
#include <utility>      // std::pair

class invertedpagetable {

 public:

    // TODO: delete lines
    // /**
    //  * @param pagesQuantity is used to store the quantity of pages
    // */
    // int pagesQuantity;
   
    /**
     * @param uses vector is used to store the quantity
     * of time a physical page is used
    */
    std::vector<int> uses;

    /**
     * @param virtualPageTable contains the "mapping" between the virtual and the physical pages
     * the virtualPageTable[i] is the virtual page
     * the i value is the physical page
    */
    std::vector<int> virtualPageTable;

    /**
     * @param fifo_queue is a queue that contains the physical pages in the order they were used
     * the first element is the first page to be replaced
    */
    std::list<int> fifo_queue;

    /**
     * @param pageCounter
    */
    int pageCounter; 

    /**
     * @param second_chance_queue is a queue that contains the physical pages in the order they were used
     * the first element is the first page to be replaced
    */
    std::list<std::pair<int, bool>> second_chance_queue;

    /**
     * @param physicalPagesBitmap
    */
    BitMap* physicalPagesBitmap;

    /**
     * @param replacementAlgorithm is a variable that contains the id of the replacement algorithm to use
    */
    int replacementAlgorithm;

    // /**
    //  * @param spaces spaces is a vector of pointers toward AddrSpaces, used to now if a given page is " owned" by a given AddrSpace
    // */
    // std::vector<AddrSpace*> spaces;

    /**
     * @param NumPhysPages_IPT
    */
    int NumPhysPages_IPT;


    /**
     * @brief invertedpagetable is the default constructor of the class, it receives no parameters
    */
    invertedpagetable(int algorithmToUse = 1) {
        NumPhysPages_IPT = NumPhysPages*2;
        uses = std::vector<int>(NumPhysPages_IPT, 0);
        virtualPageTable = std::vector<int>(NumPhysPages_IPT, -1);
        // pagesQuantity = NumPhysPages_IPT; // TODO: delete this line
        fifo_queue = std::list<int>();
        replacementAlgorithm = algorithmToUse;
        physicalPagesBitmap = new BitMap(NumPhysPages_IPT);
        pageCounter = 0;
        // TODO: initialize the vector of counters for each pages to now how many threads have access to the pages
    }

    /**
     * @brief ~invertedpagetable is the destructor of the class, it receives no parameters
    */
    ~invertedpagetable() {
        // delete physicalPagesBitmap;
    }

    TranslationEntry* getTLBEntryToReplace() {
        TranslationEntry* entryToReplace;
        switch (replacementAlgorithm) {
                
            case 0: {
                // FIFO
                break;
            }

            case 1: {

                // Second chance
                while (true) {
                    // get the first element of the queue
                    std::pair<int, bool> firstElement = second_chance_queue.front();
                    // if the page was not used
                    if (firstElement.second == false) {
                        // set the page as used
                        second_chance_queue.front().second = true;
                        // move the page to the end of the queue
                        second_chance_queue.push_back(second_chance_queue.front());
                        second_chance_queue.pop_front();
                        // get the physical page
                        entryToReplace = &machine->tlb[firstElement.first];
                        // std::cout << "PhysicalPage of the entry to replace :" << entryToReplace->physicalPage << std::endl;
                        break;
                    }
                    // if the page was used
                    else {
                        // set the page as not used
                        second_chance_queue.front().second = false;
                        // move the page to the end of the queue
                        second_chance_queue.push_back(second_chance_queue.front());
                        second_chance_queue.pop_front();
                    }
                }
                break;
            }

            case 2: {

                // get first element an set as best candidate (at least for now)
                entryToReplace = &(machine->tlb[0]);                // entry to replace is the candidate to be replaced
                int physicalPage = entryToReplace->physicalPage;    // physical page of the initial candidate
                int leastUsed = this->uses[physicalPage];           // quantity of uses of the initial candidate
                TranslationEntry* buffer = nullptr;                 // buffer to store the candidate to replace the entry to replace
                int physicalPageBuffer = 0;                         // buffer to store the physical page of the entry to replace
                int usesBuffer = 0;                                 // buffer to store the quantity of uses of the entry to replace
                
                // check all the entries in the TLB and if the entry is the least used, then set it as the entry to replace
                for (int page = 1; page < TLBSize; page++) {
                    buffer = &(machine->tlb[page]);
                    physicalPageBuffer = buffer->physicalPage;
                    usesBuffer = this->uses[physicalPageBuffer];
                    // update best candidate
                    if (usesBuffer < leastUsed) {
                        entryToReplace = buffer;
                        physicalPage = physicalPageBuffer;
                        leastUsed = usesBuffer;
                    }
                }

                // as the new entry will be replaced, the uses of the physical page should be set to 0
                this->uses[physicalPage] = 0;
                break;
            }
            
            default: {

                break;
            }
        }

        return entryToReplace;
    }

    /**
     * @brief getLeastUsedPage is a function that returns the physical page of the least used page
     * @note the function return the physical page but does not set to 0 the uses of the page,
     * this should be done by the caller using setUses function
     * @return the physical page
    */
    int getLeastUsedPhysicalPage() {
        int leastUsedPage = 0;
        int leastUses = uses[0];
        for (int i = 1; i < NumPhysPages_IPT; i++) {
            if (uses[i] < leastUses) {
                leastUses = uses[i];
                leastUsedPage = i;
            }
        }
        return leastUsedPage;
    }

    // /**
    //  * @brief setUses is a function that sets the uses of a given physical page
    //  * @param physicalPage is the physical page to set the uses
    //  * @param usesQuantity is the quantity of uses to set, usually 0
    //  * @return the physical page
    // */
    // void setUses(int physicalPage, int usesQuantity) {
    //     this->uses[physicalPage] = usesQuantity;
    // }

    // /**
    //  * @brief getUses is a function that returns the uses of a given physical page
    //  * @param physicalPage is the physical page to get the uses
    //  * @return the uses of the physical page
    // */
    // int getUses(int physicalPage) {
    //     return this->uses[physicalPage];
    // }

    // NumPhysPages_IPT is a const defined in machine.h and should be used as the const for the quantity of physical pages in the machine

    /** 
     * @brief updatePage is a function that allow the inverted page table to know that a given page was used
    */
    void updatePage(int physicalPage, bool resetUses = true) {
        // depending on the replacement algorithm a different update should be done
        switch (replacementAlgorithm)
        {
        case 0:
            // FIFO
            fifo_queue.push_back(physicalPage);
            break;
        case 1:
            // Second chance
            second_chance_queue.push_back(std::make_pair(physicalPage, true));
            // TODO: bit value should be true or false ?
            break;
        case 2: 
            // re-initialized the quantity of uses
            if(resetUses) {
                this->uses[physicalPage] = 0;
            } else {
                this->uses[physicalPage]++;
            }
            

            break;
        default:
            break;
        }
    }


    int getPhysicalPage(int virtualPage) {
        // start getting a free place in the physicalPagesBitmap
        int availablePage = 0;
        availablePage = this->physicalPagesBitmap->Find();

        if (availablePage == -1)
        {
            // std::cout << "No more physical pages available, need to swap" << std::endl;
            int positionToSwap = this->getEntryToReplace();
            if(!currentThread->space->swap(positionToSwap, this->virtualPageTable[positionToSwap])) {
                // std::cout << "Error swapping page" << std::endl;
                return -1;
            }

            availablePage = positionToSwap;
            pageCounter--;
        }

        this->virtualPageTable[availablePage] = virtualPage;
        
        // TODO: add some data structure to store the addrSpaces ?

        this->pageCounter++;
        return availablePage;
    }

    int getEntryToReplace() {
        int pageToReplace = 0;
        switch (replacementAlgorithm)
        {
        case 0:
            // FIFO
            pageToReplace = fifo_queue.front();
            fifo_queue.pop_front();
            break;
        case 1:
            // Second chance
            while (true) {
                // get the first element of the queue
                std::pair<int, bool> firstElement = second_chance_queue.front();
                // if the page was not used
                if (firstElement.second == false) {
                    // set the page as used
                    second_chance_queue.front().second = true;
                    // move the page to the end of the queue
                    second_chance_queue.push_back(second_chance_queue.front());
                    second_chance_queue.pop_front();
                    // get the physical page
                    pageToReplace = firstElement.first;
                    break;
                }
                // if the page was used
                else {
                    // set the page as not used
                    second_chance_queue.front().second = false;
                    // move the page to the end of the queue
                    second_chance_queue.push_back(second_chance_queue.front());
                    second_chance_queue.pop_front();
                }
            }
            break;
        case 2:
            // least used
            pageToReplace = this->getLeastUsedPhysicalPage();
            // delete the entry from the physical page table and the array of uses
            virtualPageTable[pageToReplace] = -1;
            uses[pageToReplace] = 0;
            break;
        default:
            break;
        }
        return pageToReplace;
    }

    void RestorePages() {
        for (int i = 0; i < TLBSize; i++) {
            // std::cout << "validating page: " << i << std::endl;
            // TODO: check if page belongs to currentThread->space
            machine->tlb[i].valid = true;
        }
    }

 private:
    /**
     * @param MAXIMUM_USES is a constant used as a limit of maximum uses
    */
    const int MAXIMUM_USES = 4096; // this value was choosen arbitrarily, could be modifed if necesarry

    /**
     * @param MAXIMUM_OWNERS is a constant used to limit the maximum quantity
     * of threads that can own a given page
    */
    const int MAXIMUM_OWNERS = 512; // this value was choosen arbitrarily, could be modifed if necesarry


};
