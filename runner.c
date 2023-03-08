#include "memstream.h"

#include <stdint.h>

#pragma pack(push, 1)
struct run_struct {
    uint8_t do_exec;
    uint64_t function_address;
    uint64_t arguments_pointer;
    uint64_t return_value;
    uint64_t single_arg_argument;
};
#pragma pack(pop)

HRESULT MSS_Runner_Malloc(PMSSFunctionRunner runner, size_t size, uint64_t* pAllocation) {
    if(!runner || !runner->process || !runner->process->ctx || !runner->process->ctx->hVMM)
        return E_UNEXPECTED;

    if(!size) return E_INVALIDARG;
    if(!pAllocation) return E_INVALIDARG;

    uint64_t mallocAddr = 0;
    HRESULT hr = MSS_GetModuleExport(runner->process,
                        "ucrtbase.dll",
                        "malloc",
                        &mallocAddr);
    if(FAILED(hr)) return hr;

    return MSS_Runner_Run(runner, mallocAddr, 1, pAllocation, size);
}