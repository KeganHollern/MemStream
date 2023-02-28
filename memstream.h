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




//--- PCIE FPGA

HRESULT MSS_InitFPGA(PMSSContext* pCtx);
HRESULT MSS_DisableMasterAbort(PMSSContext ctx);

//--- PROCESS

HRESULT MSS_GetProcess(PMSSContext ctx, const char* name, PMSSProcess* pProcess);
HRESULT MSS_GetAllProcesses(PMSSContext ctx, const char* name, PProcess* pProcessList);

HRESULT MSS_GetModuleBase(PMSSProcess process, const char* name, uint64_t* pBase);
HRESULT MSS_GetModuleExports(PMSSProcess process, const char* name, PExport* pExportList);
HRESULT MSS_GetModuleImports(PMSSProcess process, const char* name, PImport* pImportList);

HRESULT MSS_Free(void* list);

//TODO: memory read/write helpers

//TODO: memory routines (findpattern)

//TODO: freeing for linked lists

//TODO: remoteprocess rework

//TODO: remoteinput rework

//TODO: directx rework

//TODO: functionrunner rework

#endif //MEMSTREAM_MEMSTREAM_H
