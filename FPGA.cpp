//
// Created by Kegan Hollern on 12/23/23.
//
#include <cassert>
#include <vector>
#include <cstdint>
#include <string>
#include <algorithm>
#include <cctype>
#include <vmmdll.h>

#include "FPGA.h"

namespace memstream {
    // this might be retarded
    //TODO: ? global singleton ?
    FPGA* gDevice = nullptr;

    FPGA::FPGA() {
        if(gDevice == nullptr) gDevice = this;

        char* args[4] = {};
        args[0] = "";
        args[1] = "-device";
        args[2] = "fpga";

        PLC_CONFIG_ERRORINFO err = nullptr;
        this->vmm = VMMDLL_InitializeEx(3, args, &err);

        assert(this->vmm && "failed to initialize FPGA");
        //TODO: throw exception on error
    }
    FPGA::~FPGA() {
        if(gDevice == this) gDevice = nullptr;
        assert(this->vmm && "deconstructing with invalid hVMM");
        VMMDLL_Close(this->vmm);
    }

    bool FPGA::DisableMasterAbort() {
        ULONG64 qwID = 0, qwVersionMajor = 0, qwVersionMinor = 0;

        if (!VMMDLL_ConfigGet(this->vmm, LC_OPT_FPGA_FPGA_ID, &qwID) ||
            !VMMDLL_ConfigGet(this->vmm, LC_OPT_FPGA_VERSION_MAJOR, &qwVersionMajor) ||
            !VMMDLL_ConfigGet(this->vmm, LC_OPT_FPGA_VERSION_MINOR, &qwVersionMinor))
            return false;

        if (qwVersionMajor < 4 || (qwVersionMajor < 5 && qwVersionMinor < 7))
            return false; // must be version 4.7 or newer!

        HANDLE hLC;
        LC_CONFIG LcConfig = {
                .dwVersion = LC_CONFIG_VERSION,
                .szDevice = "existing"
        };
        // fetch already existing leechcore handle.
        hLC = LcCreate(&LcConfig);
        if(!hLC) return false;

        // enable auto-clear of status register [master abort].
        LcCommand(
                hLC,
                LC_CMD_FPGA_CFGREGPCIE_MARKWR | 0x002,
                4,
                (BYTE[4]) { 0x10, 0x00, 0x10, 0x00 },
                nullptr,
                nullptr);
        // close leechcore handle.
        LcClose(hLC);

        return true;
    }

    VMM_HANDLE FPGA::getVmm() {
        return vmm;
    }

    bool FPGA::GetProcessInfo(uint32_t pid, VMMDLL_PROCESS_INFORMATION& info) {
        SIZE_T cbInfo = sizeof(VMMDLL_PROCESS_INFORMATION);
        info.magic = VMMDLL_PROCESS_INFORMATION_MAGIC;
        info.wVersion = VMMDLL_PROCESS_INFORMATION_VERSION;
        return VMMDLL_ProcessGetInformation(this->vmm, pid, &info, &cbInfo);
    }

    std::vector<uint32_t> FPGA::GetAllProcessesByName(const std::string &name) {
        std::vector<uint32_t> results;

        PDWORD pdwPIDs;
        SIZE_T cPIDs = 0;

        if(!VMMDLL_PidList(this->vmm, nullptr, &cPIDs)) return results;

        // we use alloca to store tmp data on stack for perf
        pdwPIDs = (PDWORD)alloca(cPIDs * sizeof(DWORD));
        memset(pdwPIDs, 0, cPIDs * sizeof(DWORD));
        if (!VMMDLL_PidList(this->vmm, pdwPIDs, &cPIDs))
            return results;

        for (SIZE_T i = 0; i < cPIDs; i++)
        {
            DWORD dwPid = pdwPIDs[i];

            // pull info from process
            VMMDLL_PROCESS_INFORMATION info = {0};
            if(!this->GetProcessInfo(dwPid, info))
                continue;

            // case-insensitive name check
            std::string procName(info.szNameLong);
            if(std::equal(name.begin(), name.end(), procName.begin(), [](char a, char b) {
                return std::tolower(a) == std::tolower(b);
            }))
            {
                results.push_back(dwPid);
            }
        }

        return results;
    }


} // memstream