#include "memstream.h"

#include <stdint.h>
#include <Windows.h>
#include <leechcore.h>
#include <vmmdll.h>


// MSS_GetProcess returns the first matching process
HRESULT MSS_GetProcess(PMSSContext ctx, const char* name, PMSSProcess* pProcess) {
    if(!ctx || !ctx->hVMM) return E_UNEXPECTED;
    if(!name) return E_INVALIDARG;
    if(!pProcess) return E_INVALIDARG;

   DWORD pid = 0;

    if(!VMMDLL_PidGetFromName(ctx->hVMM, (char*)name, &pid))
        return E_FAIL;

    // construct process & return OK
    *pProcess = malloc(sizeof(MSSProcess));
    (*pProcess)->ctx = ctx;
    (*pProcess)->pid = pid;

    return S_OK;
}

// MSS_GetAllProcesses returns a linked list of all processes with the following name
HRESULT MSS_GetAllProcesses(PMSSContext ctx, const char* name, PProcessList* pProcessList) {
    if(!ctx || !ctx->hVMM) return E_UNEXPECTED;
    if(!name) return E_INVALIDARG;
    if(!pProcessList) return E_INVALIDARG;

    PDWORD pdwPIDs = 0;
    ULONG64 cPIDs = 0;

    if(!VMMDLL_PidList(ctx->hVMM, NULL, &cPIDs)) return E_FAIL;

    pdwPIDs = (PDWORD)LocalAlloc(LMEM_ZEROINIT, cPIDs * sizeof(DWORD));
    if (!VMMDLL_PidList(ctx->hVMM, pdwPIDs, &cPIDs))
    {
        LocalFree((HLOCAL)pdwPIDs);
        return E_FAIL;
    }

    ProcessList* current = NULL;

    for (DWORD i = 0; i < cPIDs; i++)
    {
        DWORD dwPid = pdwPIDs[i];

        VMMDLL_PROCESS_INFORMATION info = {};
        SIZE_T cbInfo = sizeof(VMMDLL_PROCESS_INFORMATION);
        info.magic = VMMDLL_PROCESS_INFORMATION_MAGIC;
        info.wVersion = VMMDLL_PROCESS_INFORMATION_VERSION;
        if (!VMMDLL_ProcessGetInformation(ctx->hVMM, dwPid, &info, &cbInfo))
        {
            continue;
        }
        if(!strcmp(info.szNameLong, name))
        {
            // push entry to end of linked list (or create head if first hit)
            ProcessList* new = (ProcessList*)malloc(sizeof(ProcessList));
            new->next = NULL;
            new->value = (PMSSProcess)(sizeof(MSSProcess));
            new->value->pid = dwPid;
            new->value->ctx = ctx;
            if(current) {
                current->next = new;
            } else {
                *pProcessList = new;
            }
            current = new;
        }
    }

    LocalFree((HLOCAL)pdwPIDs);

    return S_OK;
}

// MSS_GetModuleBase returns the base address of a module in the process
HRESULT MSS_GetModuleBase(PMSSProcess process, const char* name, uint64_t* pBase) {
    if(!process || !process->ctx || !process->ctx->hVMM) return E_UNEXPECTED;
    if(!name) return E_INVALIDARG;
    if(!pBase) return E_INVALIDARG;

    *pBase = VMMDLL_ProcessGetModuleBaseU(process->ctx->hVMM, process->pid, (char*)name);
    if(!*pBase) return E_FAIL;

    return S_OK;
}

// MSS_GetModuleExports returns a linked list of module exports from the export address table.
HRESULT MSS_GetModuleExports(PMSSProcess process, const char* name, PExportList* pExportList) {
    if(!process || !process->ctx || !process->ctx->hVMM) return E_UNEXPECTED;
    if(!name) return E_INVALIDARG;
    if(!pExportList) return E_INVALIDARG;

    PVMMDLL_MAP_EAT pEatMap = NULL;
    PVMMDLL_MAP_EATENTRY pEatMapEntry;

    ExportList* current = NULL;

    if(!VMMDLL_Map_GetEATU(process->ctx->hVMM, process->pid, (char*)name, &pEatMap))
        return E_FAIL;

    if(pEatMap->dwVersion != VMMDLL_MAP_EAT_VERSION) {
        VMMDLL_MemFree(pEatMap);
        return E_FAIL;
    }
    for(int i = 0; i < pEatMap->cMap; i++) {
        pEatMapEntry = pEatMap->pMap + i;

        ExportList* new = (ExportList*)malloc(sizeof(ExportList));
        new->next = NULL;
        new->name = malloc(strlen(pEatMapEntry->uszFunction)+1);
        strcpy_s((char*)new->name, strlen(pEatMapEntry->uszFunction)+1, pEatMapEntry->uszFunction);
        new->address = pEatMapEntry->vaFunction;
        if(current) {
            current->next = new;
        } else {
            *pExportList = new;
        }

        current = new;
    }

    VMMDLL_MemFree(pEatMap);
    return S_OK;
}

// MSS_GetModuleExport returns the address pointed to by the export with the specified name
HRESULT MSS_GetModuleExport(PMSSProcess process, const char* module, const char* export, uint64_t* pAddress) {
    PExportList exports;
    HRESULT hr = MSS_GetModuleExports(process, module, &exports);
    if(FAILED(hr)) return hr;

    ExportList* current = exports;
    while(current) {
        if(!strcmp(current->name,export)) {
            *pAddress = current->address;
            break;
        }
        current = current->next;
    }

    MSS_Free(exports);
    return S_OK;
}

// MSS_GetModuleImports returns a linked list of module imports from the import address table.
HRESULT MSS_GetModuleImports(PMSSProcess process, const char* name, PImportList* pImportList) {
    if(!process || !process->ctx || !process->ctx->hVMM) return E_UNEXPECTED;
    if(!name) return E_INVALIDARG;
    if(!pImportList) return E_INVALIDARG;

    PVMMDLL_MAP_IAT pIatMap = NULL;
    PVMMDLL_MAP_IATENTRY pIatMapEntry;

    if(!VMMDLL_Map_GetIATU(process->ctx->hVMM, process->pid, (char*)name, &pIatMap))
        return E_FAIL;

    if(pIatMap->dwVersion != VMMDLL_MAP_IAT_VERSION) {
        VMMDLL_MemFree(pIatMap);
        return E_FAIL;
    }

    ImportList* current = NULL;

    for(int i = 0; i < pIatMap->cMap; i++) {
        pIatMapEntry = pIatMap->pMap + i;

        ImportList* new = (ImportList*)malloc(sizeof(ImportList));
        new->next = NULL;
        new->name = malloc(strlen(pIatMapEntry->uszFunction)+1);
        strcpy_s((char*)new->name, strlen(pIatMapEntry->uszFunction)+1, pIatMapEntry->uszFunction);
        new->address = pIatMapEntry->vaFunction;
        if(current) {
            current->next = new;
        } else {
            *pImportList = new;
        }

        current = new;
    }

    VMMDLL_MemFree(pIatMap);
    return S_OK;
}

// MSS_GetModuleImport returns the address pointed to by the import with the specified name
HRESULT MSS_GetModuleImport(PMSSProcess process, const char* module, const char* export, uint64_t* pAddress) {
    PImportList imports;
    HRESULT hr = MSS_GetModuleImports(process, module, &imports);
    if(FAILED(hr)) return hr;

    ImportList* current = imports;
    while(current) {
        if(!strcmp(current->name,export)) {
            *pAddress = current->address;
            break;
        }
        current = current->next;
    }

    MSS_Free(imports);
    return S_OK;
}


HRESULT MSS_GetProcessModules(PMSSProcess process, PModuleList* pModuleList) {
    if(!process || !process->ctx || !process->ctx->hVMM) return E_UNEXPECTED;
    if(!pModuleList) return E_INVALIDARG;

    PVMMDLL_MAP_MODULE pModuleMap = NULL;
    PVMMDLL_MAP_MODULEENTRY pModuleEntry;

    if(!VMMDLL_Map_GetModuleU(process->ctx->hVMM, process->pid, &pModuleMap, 0))
        return E_FAIL;

    if(pModuleMap->dwVersion != VMMDLL_MAP_MODULE_VERSION) {
        VMMDLL_MemFree(pModuleMap);
        return E_FAIL;
    }

    ModuleList* current = NULL;

    for(int i = 0; i < pModuleMap->cMap;i++) {
        pModuleEntry = pModuleMap->pMap + i;

        ModuleList* new = (ModuleList*)malloc(sizeof(ModuleList));
        new->next = NULL;
        new->name = malloc(strlen(pModuleEntry->uszText)+1);
        strcpy_s((char*)new->name, strlen(pModuleEntry->uszText)+1, pModuleEntry->uszText);

        if(current) {
            current->next = new;
        } else {
            *pModuleList = new;
        }

        current = new;
    }

    VMMDLL_MemFree(pModuleMap);
    return S_OK;
}

HRESULT MSS_Is64Bit(
        PMSSProcess process,
        BOOL* pIs64) {
    return E_NOTIMPL;
}

HRESULT MSS_DumpProcess(
        PMSSProcess process,
        FILE* hDumpFile) {
    return E_NOTIMPL;
}

HRESULT MSS_DumpModule(
        PMSSProcess process,
        const char* moduleName,
        FILE* hDumpFile) {
    return E_NOTIMPL;
}
