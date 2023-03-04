#ifndef MEMSTREAM_MEMSTREAM_H
#define MEMSTREAM_MEMSTREAM_H

#include <vmmdll.h>
#include <stdint.h>

//--- STRUCTURES

// MSSContext is a global context for DMA interaction.
typedef struct MSSContext {
    VMM_HANDLE hVMM;
} MSSContext, *PMSSContext;

// MSSProcess represents a process on the remote machine.
typedef struct MSSProcess {
    PMSSContext ctx;
    uint64_t pid;
} MSSProcess, *PMSSProcess;


// Process is a linked list of processes
typedef struct ProcessList {
    struct ProcessList* next;
    PMSSProcess value;
} ProcessList, *PProcessList;

// Import is a linked list of module imports
typedef struct ImportList {
    struct ImportList* next;
    const char* name;
    uint64_t address;
} ImportList, *PImportList;

// Export is a linked list of module exports
typedef struct ExportList {
    struct ExportList* next;
    const char* name;
    uint64_t address;
} ExportList, *PExportList;

// Module is a linked list of module names
typedef struct ModuleList {
    struct ModuleList* next;
    const char* name;
} ModuleList, *PModuleList;


//--- PCIE FPGA

HRESULT MSS_InitFPGA(PMSSContext* pCtx);
HRESULT MSS_CloseContext(PMSSContext ctx);
HRESULT MSS_DisableMasterAbort(PMSSContext ctx);

//--- UTILS

void MSS_PrintBuffer(void* buffer, size_t size);

//--- PROCESS

HRESULT MSS_GetProcess(PMSSContext ctx, const char* name, PMSSProcess* pProcess);
HRESULT MSS_GetAllProcesses(PMSSContext ctx, const char* name, PProcessList* pProcessList);

HRESULT MSS_GetModuleBase(PMSSProcess process, const char* name, uint64_t* pBase);
HRESULT MSS_GetModuleExports(PMSSProcess process, const char* name, PExportList* pExportList);
HRESULT MSS_GetModuleExport(PMSSProcess process, const char* module, const char* export, uint64_t* pAddress);
HRESULT MSS_GetModuleImports(PMSSProcess process, const char* name, PImportList* pImportList);
HRESULT MSS_GetModuleImport(PMSSProcess process, const char* module, const char* export, uint64_t* pAddress);
HRESULT MSS_GetProcessModules(PMSSProcess process, PModuleList* pModuleList);


HRESULT MSS_Free(void* list);

//--- READ

HRESULT MSS_ReadSingle(PMSSProcess process, uint64_t address, void* buffer, size_t size);


// ----- ah ok

typedef struct MSSReadArray {
    int64_t* read_addresses;
    void** read_buffers;
    size_t* read_sizes;
    size_t count;
    size_t capacity;
} MSSReadArray, *PMSSReadArray;


HRESULT MSS_ReadMany(PMSSProcess process, uint64_t addresses[], void* buffers[], size_t sizes[], size_t count);

HRESULT MSS_PushRead(PMSSReadArray array, uint64_t address, void* buffer, size_t size); // add element to read array
HRESULT MSS_PushManyReads(PMSSReadArray array, uint64_t* addresses, void** buffers, size_t* sizes, size_t count);
HRESULT MSS_FreeRead(PMSSReadArray array); // delete read array
HRESULT MSS_NewReadArray(size_t capacity, PMSSReadArray* pArray); // construct read array

/*
// ReadOp is a double linked list of read operations for scatter reading
typedef struct ReadOp {
    struct ReadOp* next; // NULL if tail
    struct ReadOp* prev; // NULL if head
    void* buffer;
    int64_t address;
    size_t size;
} ReadOp, *PReadOp;



HRESULT MSS_ReadMany(PMSSProcess process, PReadOp reads);

HRESULT MSS_NewReadOp(uint64_t address, void* buffer, size_t size, PReadOp* pReadOp);
HRESULT MSS_FreeReadOp(PReadOp read);
HRESULT MSS_InsertReadOp(PReadOp parent, PReadOp read);
HRESULT MSS_CreateReadOps(uint64_t addresses[], void* buffers[], size_t sizes[], size_t count, PReadOp* pReads);
*/

//TODO: write

//TODO: memory routines (findpattern)

//TODO: freeing for linked lists

//TODO: remoteprocess rework

//TODO: remoteinput rework

//TODO: directx rework

//TODO: functionrunner rework


#endif //MEMSTREAM_MEMSTREAM_H
