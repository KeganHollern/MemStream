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

#include "MemStream/FPGA.h"

namespace memstream {
    // 99.99% of users don't need multiple
    // devices. So we have a "default" device
    // which is auto-constructed
    // whenever any of the objects (process/driver/ect.)
    // are used.
    FPGA* gDevice = nullptr;
    FPGA* GetDefaultFPGA() {
        if(!gDevice) {
            gDevice = new FPGA();
            assert(gDevice);
        }
        return gDevice;
    }

    FPGA::FPGA() {
        static char* args[4] = {};
        args[0] = "";
        args[1] = "-device";
        args[2] = "fpga";

        this->vmm = VMMDLL_Initialize(3, args);
        if(!this->vmm)
            throw std::runtime_error("failed to initialize device");
    }
    FPGA::~FPGA() {
        assert(this->vmm && "null vmm");

        VMMDLL_Close(this->vmm);
    }

    bool FPGA::DisableMasterAbort() {
        assert(this->vmm && "null vmm");

        ULONG64 qwID = 0, qwVersionMajor = 0, qwVersionMinor = 0;

        if (!VMMDLL_ConfigGet(this->vmm, LC_OPT_FPGA_FPGA_ID, &qwID) ||
            !VMMDLL_ConfigGet(this->vmm, LC_OPT_FPGA_VERSION_MAJOR, &qwVersionMajor) ||
            !VMMDLL_ConfigGet(this->vmm, LC_OPT_FPGA_VERSION_MINOR, &qwVersionMinor))
            return false;

        if (qwVersionMajor < 4 || (qwVersionMajor < 5 && qwVersionMinor < 7))
            return false; // must be version 4.7 or newer!

        LC_CONFIG LcConfig = {
                .dwVersion = LC_CONFIG_VERSION,
                .szDevice = "existing"
        };

        // fetch already existing leechcore handle.
        HANDLE hLC = LcCreate(&LcConfig);
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
        assert(this->vmm && "null vmm");

        return this->vmm;
    }

    bool FPGA::GetProcessInfo(uint32_t pid, VMMDLL_PROCESS_INFORMATION& info) {
        assert(this->vmm && "null vmm");

        SIZE_T cbInfo = sizeof(VMMDLL_PROCESS_INFORMATION);
        info.magic = VMMDLL_PROCESS_INFORMATION_MAGIC;
        info.wVersion = VMMDLL_PROCESS_INFORMATION_VERSION;
        return VMMDLL_ProcessGetInformation(this->vmm, pid, &info, &cbInfo);
    }

    std::vector<uint32_t> FPGA::GetAllProcessesByName(const std::string &name) {
        assert(this->vmm && "null vmm");

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