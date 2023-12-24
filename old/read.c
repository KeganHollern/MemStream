#include "memstream.h"

// #include <stdio.h>
#include <stdint.h>
#include <Windows.h>
#include <leechcore.h>
#include <vmmdll.h>


// MSS_ReadMany will do a single read DMA request to read many addresses
// NOTE: if you read multiple values from the same page, it's more efficient to combine them into a larger buffer.

HRESULT MSS_ReadMany(PMSSProcess process, uint64_t addresses[], void* buffers[], size_t sizes[], size_t count) {
    if(!process || !process->ctx || !process->ctx->hVMM) return E_UNEXPECTED;
    if(!addresses) return E_INVALIDARG;
    if(!buffers) return E_INVALIDARG;
    if(!sizes) return E_INVALIDARG;
    if(!count) return E_INVALIDARG;

    VMMDLL_SCATTER_HANDLE hScatter = VMMDLL_Scatter_Initialize(process->ctx->hVMM, process->pid, MSS_READ_FLAGS);

    for(size_t i = 0; i < count; i++) {
        uint64_t address = addresses[i];
        void* buffer = buffers[i];
        size_t size = sizes[i];

        if((address+size) >= page_tail(address)) {
            // TODO: determine if this needs broken into two reads
        }

        // we use Ex here so we write bytes directly the the desired buffer
        // when ExecuteRead is called - this saves us a memcpy step & boosts perf
        if(!VMMDLL_Scatter_PrepareEx(
                hScatter,
                address,
                size,
                buffer,
                NULL)) { // NULL should be Ok here because its "_out_opt_" ?
            // error preping - ensure we close the scatter handle safely
            VMMDLL_Scatter_CloseHandle(hScatter);
            return E_FAIL;
        }
    }

    if(!VMMDLL_Scatter_ExecuteRead(hScatter)) {
        // read execution failed for some reason
        VMMDLL_Scatter_CloseHandle(hScatter);
        return E_FAIL;
    }

    VMMDLL_Scatter_CloseHandle(hScatter);
    return S_OK;
}

HRESULT MSS_FindModulePattern(
        PMSSProcess process,
        const char* module,
        const char* pattern,
        uint64_t* pFound) {
    return E_NOTIMPL;
}