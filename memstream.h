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
typedef struct Process {
    struct Process* next;
    PMSSProcess value;
} Process, *PProcess;

// Import is a linked list of module imports
typedef struct Import {
    struct Import* next;
    const char* name;
    uint64_t address;
} Import, *PImport;

// Export is a linked list of module exports
typedef struct Export {
    struct Export* next;
    const char* name;
    uint64_t address;
} Export, *PExport;

// Module is a linked list of module names
typedef struct Module {
    struct Module* next;
    const char* name;
} Module, *PModule;

// ReadOp is a double linked list of read operations for scatter reading
typedef struct ReadOp {
    struct ReadOp* next; // NULL if tail
    struct ReadOp* prev; // NULL if head
    void* buffer;
    int64_t address;
    size_t size;
} ReadOp, *PReadOp;


//--- PCIE FPGA

HRESULT MSS_InitFPGA(PMSSContext* pCtx);
HRESULT MSS_DisableMasterAbort(PMSSContext ctx);

//--- PROCESS

HRESULT MSS_GetProcess(PMSSContext ctx, const char* name, PMSSProcess* pProcess);
HRESULT MSS_GetAllProcesses(PMSSContext ctx, const char* name, PProcess* pProcessList);

HRESULT MSS_GetModuleBase(PMSSProcess process, const char* name, uint64_t* pBase);
HRESULT MSS_GetModuleExports(PMSSProcess process, const char* name, PExport* pExportList);
HRESULT MSS_GetModuleImports(PMSSProcess process, const char* name, PImport* pImportList);
HRESULT MSS_GetProcessModules(PMSSProcess process, PModule* pModuleList);


HRESULT MSS_Free(void* list);

//--- READ

HRESULT MSS_ReadSingle(PMSSProcess process, uint64_t address, void* buffer, size_t size);
HRESULT MSS_ReadMany(PMSSProcess process, PReadOp reads);

HRESULT MSS_NewReadOp(uint64_t address, void* buffer, size_t size, PReadOp* pReadOp);
HRESULT MSS_InsertReadOp(PReadOp parent, PReadOp read);
HRESULT MSS_CreateReadOps(uint64_t addresses[], void* buffers[], size_t sizes[], size_t count, PReadOp* pReads);

//TODO: memory read/write helpers

//TODO: memory routines (findpattern)

//TODO: freeing for linked lists

//TODO: remoteprocess rework

//TODO: remoteinput rework

//TODO: directx rework

//TODO: functionrunner rework

#endif //MEMSTREAM_MEMSTREAM_H
