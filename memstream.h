#ifndef MEMSTREAM_MEMSTREAM_H
#define MEMSTREAM_MEMSTREAM_H

#include <vmmdll.h>
#include <stdint.h>

extern VMM_HANDLE gVMM;

//--- PCIE FPGA

HRESULT MSS_InitFPGA();
HRESULT MSS_DisableMasterAbort();

//--- PROCESS

// PID is a linked list of process IDs
typedef struct PID {
    struct PID* next;
    uint64_t value;
} PID;

// Import is a linked list of module imports
typedef struct Import {
    struct Export* next;
    const char* name;
    uint64_t address;
} Import;

// Export is a linked list of module exports
typedef struct Export {
    struct Export* next;
    const char* name;
    uint64_t address;
} Export;

HRESULT MSS_GetProcessId(const char* name, uint64_t* pPid);
HRESULT MSS_GetAllProcessIds(const char* name, PID** ppPidList);
HRESULT MSS_GetModuleBase(uint64_t pid, const char* name, uint64_t* pBase);
HRESULT MSS_GetModuleExports(uint64_t pid, const char* name, Export** ppExports);

//TODO: memory read/write helpers

//TODO: memory routines (findpattern)

//TODO: freeing for linked lists

//TODO: remoteprocess rework

//TODO: remoteinput rework

//TODO: directx rework

//TODO: functionrunner rework

#endif //MEMSTREAM_MEMSTREAM_H
