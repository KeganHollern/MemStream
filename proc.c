#include "memstream.h"

#include <stdint.h>
#include <Windows.h>
#include <leechcore.h>
#include <vmmdll.h>

// createExport constructs a new export pointer in the heap for linked list allocation
Export* createExport(const char* name, uint64_t value) {
    Export* new = (Export*)malloc(sizeof(Export));
    new->next = NULL;
    new->type = MSS_LIST_EXPORTS;
    new->name = malloc(strlen(name)+1);
    strcpy_s(new->name, strlen(name)+1, name);
    new->address = value;
    return new;
}

// createImport constructs a new import pointer in the heap for linked list allocation
Import* createImport(const char* name, uint64_t value) {
    Import* new = (Import*)malloc(sizeof(Import));
    new->next = NULL;
    new->type = MSS_LIST_IMPORTS;
    new->name = malloc(strlen(name)+1);
    strcpy_s(new->name, strlen(name)+1, name);
    new->address = value;
    return new;
}



// MSS_GetProcessId returns the first matching process ID
HRESULT MSS_GetProcessId(const char* name, uint64_t* pPid) {
    if(!gVMM) return E_UNEXPECTED;
    if(!name) return E_INVALIDARG;
    if(!pPid) return E_INVALIDARG;

    *pPid = 0; // zero out ppid before feeding it in so upper bits remain zero

    if(!VMMDLL_PidGetFromName(gVMM, name, pPid))
        return E_FAIL;

    return S_OK;
}

// MSS_GetAllProcessIds returns a linked list of all process ids with the following name



HRESULT MSS_GetAllProcessIds(const char* name, PID** ppPidList) {
    if(!gVMM) return E_UNEXPECTED;
    if(!ppPidList) return E_INVALIDARG;
    if(!name) return E_INVALIDARG;

    PDWORD pdwPIDs = 0;
    ULONG64 cPIDs = 0;

    if(!VMMDLL_PidList(gVMM, NULL, &cPIDs)) return E_FAIL;

    pdwPIDs = (PDWORD)LocalAlloc(LMEM_ZEROINIT, cPIDs * sizeof(DWORD));
    if (!VMMDLL_PidList(gVMM, pdwPIDs, &cPIDs))
    {
        LocalFree((HLOCAL)pdwPIDs);
        return E_FAIL;
    }

    PID* current = NULL;

    for (DWORD i = 0; i < cPIDs; i++)
    {
        DWORD dwPid = pdwPIDs[i];

        VMMDLL_PROCESS_INFORMATION info = {};
        SIZE_T cbInfo = sizeof(VMMDLL_PROCESS_INFORMATION);
        info.magic = VMMDLL_PROCESS_INFORMATION_MAGIC;
        info.wVersion = VMMDLL_PROCESS_INFORMATION_VERSION;
        if (!VMMDLL_ProcessGetInformation(gVMM, dwPid, &info, &cbInfo))
        {
            continue;
        }
        if(!strcmp(info.szNameLong, name))
        {
            // push entry to end of linked list (or create head if first hit)
           PID* new = (PID*)malloc(sizeof(PID));
           new->next = NULL;
           new->type = MSS_LIST_PID;
            new->value = dwPid;
           if(current) {
               current->next = new;
           } else {
               *ppPidList = new;
           }
           current = new;
        }
    }

    LocalFree((HLOCAL)pdwPIDs);

    return S_OK;
}

HRESULT MSS_GetModuleBase(uint64_t pid, const char* name, uint64_t* pBase) {
    if(!gVMM) return E_UNEXPECTED;
    if(!pid) return E_INVALIDARG;
    if(!name) return E_INVALIDARG;
    if(!pBase) return E_INVALIDARG;

    *pBase = VMMDLL_ProcessGetModuleBaseU(gVMM, pid, name);
    if(!*pBase) return E_FAIL;

    return S_OK;
}

HRESULT MSS_GetModuleExports(uint64_t pid, const char* name, Export** ppExports) {
    if(!pid) return E_INVALIDARG;
    if(!name) return E_INVALIDARG;
    if(!ppExports) return E_INVALIDARG;

    PVMMDLL_MAP_EAT pEatMap = NULL;
    PVMMDLL_MAP_EATENTRY pEatMapEntry;

    Export* current = NULL;

    if(!VMMDLL_Map_GetEATU(gVMM, pid, name, &pEatMap))
        return E_FAIL;

    if(pEatMap->dwVersion != VMMDLL_MAP_EAT_VERSION) {
        VMMDLL_MemFree(pEatMap);
        return E_FAIL;
    }
    for(int i = 0; i < pEatMap->cMap; i++) {
        pEatMapEntry = pEatMap->pMap + i;

        Export* new = createExport(pEatMapEntry->uszFunction, pEatMapEntry->vaFunction);
        if(current) {
            current->next = new;
        } else {
            *ppExports = new;
        }

        current = new;
    }

    VMMDLL_MemFree(pEatMap);
    return S_OK;
}

HRESULT MSS_GetModuleImports(uint64_t pid, const char* name, Import** ppImports) {
    if(!pid) return E_INVALIDARG;
    if(!name) return E_INVALIDARG;
    if(!ppImports) return E_INVALIDARG;

    PVMMDLL_MAP_IAT pIatMap = NULL;
    PVMMDLL_MAP_IATENTRY pIatMapEntry;

    if(!VMMDLL_Map_GetIATU(gVMM, pid, name, &pIatMap))
        return E_FAIL;

    if(pIatMap->dwVersion != VMMDLL_MAP_IAT_VERSION) {
        VMMDLL_MemFree(pIatMap);
        return E_FAIL;
    }

    Import* current = NULL;

    for(int i = 0; i < pIatMap->cMap; i++) {
        pIatMapEntry = pIatMap->pMap + i;

        Import* new = createImport(pIatMapEntry->uszFunction, pIatMapEntry->vaFunction);
        if(current) {
            current->next = new;
        } else {
            *ppImports = new;
        }

        current = new;
    }

    VMMDLL_MemFree(pIatMap);
    return S_OK;
}