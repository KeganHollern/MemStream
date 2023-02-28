#include "memstream.h"

#include <Windows.h>
#include <leechcore.h>
#include <vmmdll.h>


// MSS_InitFPGA initializes VMMDLL for an FPGA device.
HRESULT MSS_InitFPGA(PMSSContext* pCtx) {
    char* args[4] = {};
    args[0] = "";
    args[1] = "-device";
    args[2] = "fpga";

    PLC_CONFIG_ERRORINFO err = 0;
    VMM_HANDLE hVMM = VMMDLL_InitializeEx(3, args, &err);
    if(!hVMM) return E_FAIL;

    // construct context object & return OK
    *pCtx = malloc(sizeof(MSSContext));
    (*pCtx)->hVMM = hVMM;

    return S_OK;
}

// MSS_DisableMasterAbort configures the PCIe config space status register flags to auto-clear.
HRESULT MSS_DisableMasterAbort(PMSSContext ctx) {
    ULONG64 qwID = 0, qwVersionMajor = 0, qwVersionMinor = 0;

    if (!VMMDLL_ConfigGet(ctx->hVMM, LC_OPT_FPGA_FPGA_ID, &qwID) ||
        !VMMDLL_ConfigGet(ctx->hVMM, LC_OPT_FPGA_VERSION_MAJOR, &qwVersionMajor) ||
        !VMMDLL_ConfigGet(ctx->hVMM, LC_OPT_FPGA_VERSION_MINOR, &qwVersionMinor)) return E_FAIL;

    if (qwVersionMajor < 4 || (qwVersionMajor < 5 && qwVersionMinor < 7))
        return E_FAIL; // must be version 4.7 or newer!

    HANDLE hLC;
    LC_CONFIG LcConfig = {
            .dwVersion = LC_CONFIG_VERSION,
            .szDevice = "existing"
    };
    // fetch already existing leechcore handle.
    hLC = LcCreate(&LcConfig);
    if(!hLC) return E_FAIL;

    // enable auto-clear of status register [master abort].
    LcCommand(
            hLC,
            LC_CMD_FPGA_CFGREGPCIE_MARKWR | 0x002,
            4,
            (BYTE[4]) { 0x10, 0x00, 0x10, 0x00 },
            NULL,
            NULL);
    // close leechcore handle.
    LcClose(hLC);

    return S_OK;
}