#include "memstream.h"

#include <stdint.h>
#include <Windows.h>
#include <leechcore.h>
#include <vmmdll.h>


// MSS_GetProcessId returns the first matching process ID
HRESULT MSS_GetProcessId(const char* name, uint64_t* pPid) {
    if(!gVMM) return E_UNEXPECTED;
    if(!name) return E_INVALIDARG;
    if(!pPid) return E_INVALIDARG;

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

        Export* new = (Export*)malloc(sizeof(Export));
        new->name = malloc(strlen(pEatMapEntry->uszFunction)+1);
        strcpy_s(new->name, strlen(pEatMapEntry->uszFunction)+1, pEatMapEntry->uszFunction);
        new->address = pEatMapEntry->vaFunction;
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

    //TODO: https://github.com/ufrisk/MemProcFS/blob/master/vmm_example/vmmdll_example.c#L956
    return E_NOTIMPL;
}