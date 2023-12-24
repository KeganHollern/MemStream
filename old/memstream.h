#ifndef MEMSTREAM_MEMSTREAM_H
#define MEMSTREAM_MEMSTREAM_H

#include <vmmdll.h>
#include <stdint.h>

//========== STRUCTURES ===========

// MSSContext is a global context for DMA
// interaction.
typedef struct MSSContext {
    VMM_HANDLE hVMM;
} MSSContext, *PMSSContext;

// MSSProcess represents a process on the
// remote machine.
typedef struct MSSProcess {
    PMSSContext ctx;
    uint64_t pid;
} MSSProcess, *PMSSProcess;

// MSSKernel represents the windows session
// process on the remote machine.
// From MSSKernel you can read/write data
// in session drivers.
typedef struct MSSKernel {
    PMSSProcess csrss;

    uint64_t _gafAsyncKeyState;
    uint64_t _gptCursorAsync;
} MSSKernel, *PMSSKernel;

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

// MSSDataArray is an array of addresses,
// buffers, and buffer sizes. This is used
// for performant reads and writes.
typedef struct MSSDataArray {
    uint64_t* addresses;
    void** buffers;
    size_t* sizes;
    size_t count;
    size_t capacity;
} MSSDataArray, *PMSSDataArray;

// MSSFunctionRunner holds data necessary
// to install and execute arbitrary code
// within the target process.
typedef struct MSSFunctionRunner {
    PMSSProcess process;
    uint64_t data_address; // location of unused .data buffer
    uint64_t call_address; // location of 5 byte relcall.
    uint64_t text_address; // location of unused .text executable code (or code cave)
} MSSFunctionRunner, *PMSSFunctionRunner;

//========== PCIE FPGA ===========

// MSS_InitFPGA initializes the DMA device and
// returns an MSSContext.
HRESULT MSS_InitFPGA(PMSSContext* pCtx);

// MSS_CloseContext closes the MSSContext and
// releases the underlying DMA device handles.
HRESULT MSS_CloseContext(PMSSContext ctx);

// MSS_DisableMasterAbort sets auto-clear for
// the master abort flag on the DMA device.
HRESULT MSS_DisableMasterAbort(PMSSContext ctx);

//========= UTILS =========

// MSS_PrintBuffer is a utility for printing a
// large byte buffer in a pretty way.
void MSS_PrintBuffer(void* buffer, size_t size);

//=========== PROCESS ==========

// MSS_GetProcess acquires the remote process
// and returns an MSSProcess.
HRESULT MSS_GetProcess(
        PMSSContext ctx,
        const char* name,
        PMSSProcess* pProcess);

// MSS_GetAllProcesses returns a list of all
// remote processes matching the provided name.
HRESULT MSS_GetAllProcesses(
        PMSSContext ctx,
        const char* name,
        PProcessList* pProcessList);

// MSS_GetModuleBase returns the base address
// of the module within the remote process.
HRESULT MSS_GetModuleBase(
        PMSSProcess process,
        const char* name,
        uint64_t* pBase);

// MSS_GetModuleExports returns a list of all
// exported symbols from the provided module.
HRESULT MSS_GetModuleExports(
        PMSSProcess process,
        const char* name,
        PExportList* pExportList);

// MSS_GetModuleExport returns the address of
// the provided exported symbol.
HRESULT MSS_GetModuleExport(
        PMSSProcess process,
        const char* module,
        const char* export,
        uint64_t* pAddress);

// MSS_GetModuleImports returns a list of all
// imported symbols in the provided module.
HRESULT MSS_GetModuleImports(
        PMSSProcess process,
        const char* name,
        PImportList* pImportList);

// MSS_GetModuleImport returns the address of
// the imported symbol in the provided module.
HRESULT MSS_GetModuleImport(
        PMSSProcess process,
        const char* module,
        const char* export,
        uint64_t* pAddress);

// MSS_GetProcessModules returns a list of all
// module names in the process.
HRESULT MSS_GetProcessModules(
        PMSSProcess process,
        PModuleList* pModuleList);

//========= READ ==========

// MSS_ReadSingle reads a single address.
// This is unoptimized and best used sparingly.
HRESULT MSS_ReadSingle(
        PMSSProcess process,
        uint64_t address,
        void* buffer,
        size_t size);

// MSS_ReadMany reads many addresses.
// This is optimized. It is better to call
// MSS_ReadMany once with many addresses than
// to call MSS_ReadSingle many times.
HRESULT MSS_ReadMany(
        PMSSProcess process,
        uint64_t addresses[],
        void* buffers[],
        size_t sizes[],
        size_t count);

//========== WRITE ==========

// MSS_WriteSingle writes a single address.
// This is not optimized.
HRESULT MSS_WriteSingle(
        PMSSProcess process,
        uint64_t address,
        void* buffer,
        size_t size);

// MSS_WriteMany writes many addresses at
// once. This is optimized.
HRESULT MSS_WriteMany(
        PMSSProcess process,
        uint64_t addresses[],
        void* buffers[],
        size_t sizes[],
        size_t count);

//--- MEMORY ROUTINES

// MSS_FindModulePattern finds the provided
// pattern within the process module.
// Pattern format is 'AA ? AA ? ? ? BB BB BB'
HRESULT MSS_FindModulePattern(
        PMSSProcess process,
        const char* module,
        const char* pattern,
        uint64_t* pFound);

//--- MEMORY MANAGEMENT

// MSS_Push pushes a read or write
// operation to an MSSReadArray.
// MSSReadArray must have capacity.
HRESULT MSS_Push(
        PMSSDataArray array,
        uint64_t address,
        void* buffer,
        size_t size);

// MSS_PushMany pushes many read or
// write operations to an MSSReadArray.
// MSSReadArray must have capacity.
HRESULT MSS_PushMany(
        PMSSDataArray array,
        uint64_t addresses[],
        void* buffers[],
        size_t sizes[],
        size_t count);

// MSS_Free releases an MSSDataArray.
HRESULT MSS_FreeArray(PMSSDataArray array);

// MSS_NewReadArray constructs an
// MSSReadArray with a fixed capacity.
HRESULT MSS_NewReadArray(
        size_t capacity,
        PMSSDataArray* pArray);

// MSS_Free releases a linked list.
HRESULT MSS_Free(void* list);

//--- PROCESS UTILS

// MSS_Is64Bit checks if the process is
// 64 or 32 bit.
HRESULT MSS_Is64Bit(
        PMSSProcess process,
        BOOL* pIs64);

// MSS_DumpProcess writes all memory
// to the provided file handle.
HRESULT MSS_DumpProcess(
        PMSSProcess process,
        FILE* hDumpFile);

// MSS_DumpModule writes all memory,
// within the bounds of the provided
// module, to the provided file handle.
HRESULT MSS_DumpModule(
        PMSSProcess process,
        const char* moduleName,
        FILE* hDumpFile);

//--- KERNEL UTILS
// TODO: can we find BEDaisy.sys with the session process?
HRESULT MSS_GetKernel(PMSSContext ctx, PMSSKernel * pOutKernel);
HRESULT MSS_GetCursorPos(PMSSKernel kernel, POINT* pOutPos);
HRESULT MSS_GetKeyState(PMSSKernel kernel, int vk, BOOL* pOutIsDown);

//--- RCE UTILS

HRESULT MSS_InitRunner(PMSSProcess process, uint64_t data_address, uint64_t call_address, uint64_t text_address, PMSSFunctionRunner* pOutRunner);

HRESULT MSS_Runner_Malloc(PMSSFunctionRunner runner, size_t size, uint64_t* pAllocation);
HRESULT MSS_Runner_Free(PMSSFunctionRunner runner, uint64_t address);
HRESULT MSS_Runner_Run(PMSSFunctionRunner runner, uint64_t function, uint64_t* pReturn, uint32_t count, ...);


#endif //MEMSTREAM_MEMSTREAM_H
