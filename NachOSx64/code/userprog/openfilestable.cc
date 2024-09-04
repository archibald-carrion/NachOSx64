#include "openfilestable.h"

NachosOpenFilesTable::NachosOpenFilesTable() {
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

NachosOpenFilesTable::~NachosOpenFilesTable() {
    delete openFilesMap;
    delete[] openFiles;
    usage = 0;
}

int NachosOpenFilesTable::Open(int UnixHandle) {
    int handle = 0;

    for (int i = 0; i < tableSize; ++i) {
        if (!openFilesMap->Test(i)) {
            continue;
        }

        if (openFiles[i] == UnixHandle) {
            handle = i;
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

int NachosOpenFilesTable::Close(int NachosHandle) {
    bool errorCon = !isOpened(NachosHandle) || NachosHandle < 0 || NachosHandle > tableSize;
    if (errorCon) {
        return -1;
    }
    int UnixHandle = openFiles[NachosHandle];
    openFilesMap->Clear(NachosHandle);
    openFiles[NachosHandle] = -1;
    return UnixHandle;
}

bool NachosOpenFilesTable::isOpened(int NachosHandle) {
    bool fileNotOpen = NachosHandle < 0 || NachosHandle > tableSize;
    if (fileNotOpen) {
        return false;
    }
    return openFilesMap->Test(NachosHandle);
}

int NachosOpenFilesTable::getUnixHandle(int NachosHandle)
{
    bool errorCon = NachosHandle < 0 || NachosHandle > tableSize;
    if (errorCon) {
        return -1;
    }
    return openFiles[NachosHandle];
}

void NachosOpenFilesTable::addThread() {
    usage++;
}

int NachosOpenFilesTable::delThread() {
    usage--;
    return usage;
}

void NachosOpenFilesTable::Print() {
    for (int file = 0; file < tableSize; file++)
    {
        if (isOpened(file))
        {
            printf("%i, %i\n", file, openFiles[file]);
        }
    }
}























// NachosOpenFilesTable::NachosOpenFilesTable() {
//     // MAX_OPEN_FILES
//     openFilesMap = new BitMap(MAX_OPEN_FILES);
//     this->usage = 0;
// }

// NachosOpenFilesTable::~NachosOpenFilesTable() {
//     delete openFilesMap;
// }

// int NachosOpenFilesTable::Open(  int UnixHandle) {
//     // se the next free position in the vector as the UnixHandle
//     int index = 3;
//     while (index < MAX_OPEN_FILES) {
//         index++;
//         printf("index: %d\n", index);
//         if (!openFilesMap->Test(index))
//         {
//             openFilesMap->Mark(index);
//             openFiles[index][0] = UnixHandle;
//             // openFiles[index][1] = family;
//             // openFiles[index][2] = type;

//             return index;
//         }
//     }
//     return -1;
// }

// int NachosOpenFilesTable::Close( int NachosHandle ) {
//     // clear the position in the vector
//     printf("NachosHandle_Close: %d\n", NachosHandle);
//     if (openFilesMap->Test(NachosHandle)) {
//         openFilesMap->Clear(NachosHandle);
//         return 1;
//     }
//     return -1;
// }

// bool NachosOpenFilesTable::isOpened( int NachosHandle ) {
//   printf("NachosHandle_isOpened: %d\n", NachosHandle);
//     return openFilesMap->Test(NachosHandle);
// }

// int NachosOpenFilesTable::getUnixHandle( int NachosHandle ) {
//   printf("NachosHandle_getUnixHandle: %d\n", NachosHandle);
//     if(openFilesMap->Test(NachosHandle)) {
//         return openFiles[NachosHandle][0];
//     }
//     return -1;
// }

// int NachosOpenFilesTable::getSockType( int NachosHandle ) {
//   printf("NachosHandle_getSockType: %d\n", NachosHandle);
//     if(openFilesMap->Test(NachosHandle)) {
//         return openFiles[NachosHandle][2];
//     }
//     return -1;
// }

// int NachosOpenFilesTable::getSockFamily( int NachosHandle ) {
//   printf("NachosHandle_getSockFamily: %d\n", NachosHandle);
//     if(openFilesMap->Test(NachosHandle)) {
//         return openFiles[NachosHandle][1];
//     }
//     return -1;
// }

// void NachosOpenFilesTable::addThread() {
//     ++this->usage;
// }

// void NachosOpenFilesTable::delThread() {
//     --this->usage;
//     if(this->usage == 0) {
//         for (int i = 0; i < MAX_OPEN_FILES; i++) {
//             if(this->openFilesMap->Test(i)) {
//                 // clean the table for i
//                 // this->openFiles[i].~OpenFile();
//                 // this->openFilesMap->Clear(i);
//             }
//             // TODO: check if this is correct
//                 	    //} else {
//                         //    break;
//                         //}

//         }
//     }
// }

// void NachosOpenFilesTable::Print() {
//     // print every element in the table
//     for (int i = 0; i < MAX_OPEN_FILES; ++i) {
//         //if (this->openFilesMap->test(i)) {
//             printf("NachosHandle: %d, UnixHandle: %ld \n", i, reinterpret_cast<long>(this->openFiles[i]));

//         //} else {
//         //    break;
//         //}
//     }
// }

// // int NachosOpenFilesTable::Close( int NachosHandle ) {
// //     // clear the position in the vector
// //     if (openFilesMap->Test(NachosHandle)) {
// //         openFilesMap->Clear(NachosHandle);
// //         return 1;
// //     }
// //     return -1;
// // }

// // bool NachosOpenFilesTable::isOpened( int NachosHandle ) {
// //     return openFilesMap->Test(NachosHandle);
// // }

// // int NachosOpenFilesTable::getUnixHandle( int NachosHandle ) {
// //     if(openFilesMap->test(NachosHandle)) {
// //         return openFiles[NachosHandle];
// //     }
// //     return -1;
// // }

// // void NachosOpenFilesTable::addThread() {
// //     ++this->usage;
// // }

// // void NachosOpenFilesTable::delThread() {
// //     --this->usage;
// //     if(this->usage == 0) {
// //         for (int i = 0; i < MAX_OPEN_FILES; i++) {
// //                         // if(this->openFilesMap->Test(i)) {
// //             // TODO: check if this is correct
// //             // this->openFiles[i].~OpenFile();
// //             // this->openFilesMap->Clear(i);
// //                 	    //} else {
// //                         //    break;
// //                         //}
// //         }
// //     }
// // }

// // void NachosOpenFilesTable::Print() {
// //     // print every element in the table
// //     for (int i = 0; i < MAX_OPEN_FILES; ++i) {
// //         //if (this->openFilesMap->test(i)) {
// //             printf("NachosHandle: %d, UnixHandle: %d\n", i, this->openFiles[i]);
// //         //} else {
// //         //    break;
// //         //}
// //     }
// // // }

// // NachosProcessTable::NachosProcessTable() {
// //     // this->nachosProcessTable = new List();
// //     this->nextFree = 0;
// // }

// // NachosProcessTable::~NachosProcessTable() {
// //     // TODO: clean every item in the array
// // }

// // int NachosProcessTable::AddProcess() {
// //     // use nextFree as new process space
// //     nachosProcessTable[nextFree] = new processData();
// //     nachosProcessTable[nextFree]->processSemaphore = new Semaphore("new semaphore", 1);
// //     nachosProcessTable[nextFree]->returnValue = 0;
// //     ++nextFree;
// //     return 0;
// // }

// // void NachosProcessTable::DeleteProcess(int process) {

// // }

// // void NachosProcessTable::wait(int process){

// // }

// // void NachosProcessTable::signal(int process) {

// // }
