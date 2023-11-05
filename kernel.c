#include "memstream.h"


HRESULT MSS_GetKernel(PMSSContext ctx, PMSSKernel * pOutKernel) {
    if(!ctx || !ctx->hVMM) return E_UNEXPECTED;
    if(!pOutKernel) return E_FAIL;
    *pOutKernel = NULL;

    PProcessList list = NULL;
    HRESULT hr = MSS_GetAllProcesses(ctx, "csrss.exe", &list);
    if(FAILED(hr)) return hr;

    if(!list) return E_FAIL;

    // walk to the end of the linked list
    while(list->next) {
        list = list->next;
    }

    // hacky way of copying the process we want
    // so that Free can be called
    PMSSProcess kernel_process = (PMSSProcess)malloc(sizeof(MSSProcess));
    ZeroMemory(kernel_process, sizeof(MSSProcess));
    memcpy(list->value, kernel_process, sizeof(MSSProcess));
    hr = MSS_Free(list);
    if(FAILED(hr)) return hr;

    *pOutKernel = (PMSSKernel)malloc(sizeof(MSSKernel));
    (*pOutKernel)->csrss = kernel_process;

    hr = MSS_GetModuleExport(kernel_process, "win32kbase.sys", "gafAsyncKeyState", &((*pOutKernel)->_gafAsyncKeyState));
    if(FAILED(hr)) return hr;
    hr = MSS_GetModuleExport(kernel_process, "win32kbase.sys", "gptCursorAsync", &((*pOutKernel)->_gptCursorAsync));
    if(FAILED(hr)) return hr;

    return S_OK;
}
HRESULT MSS_GetCursorPos(PMSSKernel kernel, POINT* pOutPos) {
    if(!kernel || !kernel->csrss || !kernel->csrss->ctx || !kernel->csrss->ctx->hVMM)
        return E_UNEXPECTED;
    if(!pOutPos) return E_INVALIDARG;

    return MSS_ReadSingle(kernel->csrss, kernel->_gptCursorAsync, pOutPos, sizeof(POINT));
}
HRESULT MSS_GetKeyState(PMSSKernel kernel, int vk, BOOL* pOutIsDown) {
    return E_NOTIMPL;
}
