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


//--- READ

HRESULT MSS_ReadSingle(PMSSProcess process, uint64_t address, void* buffer, size_t size);

typedef struct MSSReadArray {
    uint64_t* read_addresses;
    void** read_buffers;
    size_t* read_sizes;
    size_t count;
    size_t capacity;
} MSSReadArray, *PMSSReadArray;

HRESULT MSS_ReadMany(PMSSProcess process, uint64_t addresses[], void* buffers[], size_t sizes[], size_t count);

HRESULT MSS_PushRead(PMSSReadArray array, uint64_t address, void* buffer, size_t size); // add element to read array
HRESULT MSS_PushManyReads(PMSSReadArray array, uint64_t addresses[], void* buffers[], size_t sizes[], size_t count);
HRESULT MSS_FreeRead(PMSSReadArray array); // delete read array
HRESULT MSS_NewReadArray(size_t capacity, PMSSReadArray* pArray); // construct read array

//--- WRITE

HRESULT MSS_WriteSingle(PMSSProcess process, uint64_t addres, void* buffer, size_t size);
HRESULT MSS_WriteMany(PMSSProcess process, uint64_t addresses[], void* buffers[], size_t sizes[], size_t count);

//--- MEMORY ROUTINES

// MSS_FindModulePattern finds the provided pattern within the process module.
// Pattern format is 'AA ? AA ? ? ? BB BB BB'
HRESULT MSS_FindModulePattern(PMSSProcess process, const char* name, const char* pattern, uint64_t* pFound);

//--- MEMORY MANAGEMENT
HRESULT MSS_Free(void* list);

//--- PROCESS UTILS
HRESULT MSS_Is64Bit(PMSSProcess process, BOOL* pIs64);
HRESULT MSS_DumpProcess(PMSSProcess process, FILE* hDumpFile);
HRESULT MSS_DumpModule(PMSSProcess process, const char* moduleName, FILE* hDumpFile);


//--- KERNEL UTILS
// TODO: may want to make a kernel wrapper to abstract the sessionprocess logic....
// TODO: can we find BEDaisy.sys with the session process?
HRESULT MSS_GetSessionProcess(PMSSContext ctx, PMSSProcess* pOutSession);
HRESULT MSS_GetCursorPos(PMSSProcess sessionProcess, POINT* pOutPos);
HRESULT MSS_GetKeyState(PMSSProcess sessionProcess, int vk, BOOL* pOutIsDown);

//--- DIRECTX UTILS
//viewport definitions for each version
typedef struct D3D11_VIEWPORT {
    FLOAT TopLeftX;
    FLOAT TopLeftY;
    FLOAT Width;
    FLOAT Height;
    FLOAT MinDepth;
    FLOAT MaxDepth;
} D3D11_VIEWPORT;
typedef D3D11_VIEWPORT D3D12_VIEWPORT;
typedef struct D3DVIEWPORT9 {
    uint32_t X;
    uint32_t Y;
    uint32_t Width;
    uint32_t Height;
    float MinZ;
    float MaxZ;
} D3DVIEWPORT9;
enum EDirectXVersion {
    DX9,
    DX11,
    DX12,
    UNK
};
typedef struct DirectX {
    PMSSProcess  process;
    enum EDirectXVersion version;
} DirectX, *PDirectX;
HRESULT MSS_GetDirectX(PMSSProcess process, DirectX* pOutDirectX);
HRESULT MSS_GetViewport(PDirectX, uint64_t dxdevice, void** pOutViewport);


//--- RCE UTILS

typedef struct MSSFunctionRunner {
    PMSSProcess process;
    uint64_t data_address; // location of unused .data buffer
    uint64_t call_address; // location of 5 byte relcall.
    uint64_t text_address; // location of unused .text executable code (or code cave)
} MSSFunctionRunner, *PMSSFunctionRunner;

HRESULT MSS_InitRunner(PMSSProcess process, uint64_t data_address, uint64_t call_address, uint64_t text_address, PMSSFunctionRunner* pOutRunner);

HRESULT MSS_Runner_Malloc(PMSSFunctionRunner runner, size_t size, uint64_t* pAllocation);
HRESULT MSS_Runner_Free(PMSSFunctionRunner runner, uint64_t address);
HRESULT MSS_Runner_Run(PMSSFunctionRunner runner, uint64_t function, uint64_t* pReturn, uint32_t count, ...);


#endif //MEMSTREAM_MEMSTREAM_H
