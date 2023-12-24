#include "memstream.h"

#include <stdint.h>
#include <Windows.h>
#include <leechcore.h>
#include <vmmdll.h>



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
