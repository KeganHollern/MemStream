#ifndef MEMSTREAM_MEMSTREAM_H
#define MEMSTREAM_MEMSTREAM_H

#include <vmmdll.h>
#include <stdint.h>

extern VMM_HANDLE gVMM;

//--- ENUMERATIONS

enum MSS_LIST {MSS_LIST_PID, MSS_LIST_IMPORTS, MSS_LIST_EXPORTS};

//--- STRUCTURES

// PID is a linked list of process IDs
typedef struct PID {
    struct PID* next;
    enum MSS_LIST type;
    uint64_t value;
} PID;

// Import is a linked list of module imports
typedef struct Import {
    struct Import* next;
    enum MSS_LIST type;
    const char* name;
    uint64_t address;
} Import;

// Export is a linked list of module exports
typedef struct Export {
    struct Export* next;
    enum MSS_LIST type;
    const char* name;
    uint64_t address;
} Export;


//--- PCIE FPGA

HRESULT MSS_InitFPGA();
HRESULT MSS_DisableMasterAbort();

//--- PROCESS





HRESULT MSS_GetProcessId(const char* name, uint64_t* pPid);
HRESULT MSS_GetAllProcessIds(const char* name, PID** ppPidList);
HRESULT MSS_GetModuleBase(uint64_t pid, const char* name, uint64_t* pBase);
HRESULT MSS_GetModuleExports(uint64_t pid, const char* name, Export** ppExports);
HRESULT MSS_GetModuleImports(uint64_t pid, const char* name, Import** ppImports);

HRESULT MSS_Free(void* list);

//TODO: memory read/write helpers

//TODO: memory routines (findpattern)

//TODO: freeing for linked lists

//TODO: remoteprocess rework

//TODO: remoteinput rework

//TODO: directx rework

//TODO: functionrunner rework

#endif //MEMSTREAM_MEMSTREAM_H
