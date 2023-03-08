#include "memstream.h"

// #include <stdio.h>
#include <stdint.h>
#include <Windows.h>
#include <leechcore.h>
#include <vmmdll.h>

// page_head takes any address and returns the first address in the same page on a 64-bit Windows machine
uint64_t page_head(uint64_t address) {
    return address & (UINT64_MAX << 12);
}
// page_tile takes any address and returns the first address in the next page on a 64-bit Windows machine
uint64_t page_tail(uint64_t address) {
    return page_head(address) + 0x1000;
}
uint64_t page_offset(uint64_t address) {
    return address & 0xFFF;
}

ULONG64 MSS_READ_FLAGS = VMMDLL_FLAG_NO_PREDICTIVE_READ | VMMDLL_FLAG_NOCACHE | VMMDLL_FLAG_NOPAGING | VMMDLL_FLAG_NOCACHEPUT;


// MSS_ReadSingle reads a single address into a buffer.
HRESULT MSS_ReadSingle(PMSSProcess process, uint64_t address, void* buffer, size_t size) {
    if(!process || !process->ctx || !process->ctx->hVMM) return E_UNEXPECTED;
    if(!buffer) return E_INVALIDARG;
    if(!size) return E_INVALIDARG;

    DWORD read = 0;
    if(!VMMDLL_MemReadEx(
            process->ctx->hVMM,
            process->pid,
            address,
            buffer,
            size,
            &read,
            MSS_READ_FLAGS))
        return E_FAIL;

    return S_OK;
}

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
