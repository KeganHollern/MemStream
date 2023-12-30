#include <cassert>
#include <vector>
#include <cstring>
#include <algorithm>
#include <cctype>
#include <stdexcept>

#include <vmmdll.h>

#include "MemStream/FPGA.h"

namespace memstream {
    // 99.99% of users don't need multiple
    // devices. So we have a "default" device
    // which is auto-constructed
    // whenever any of the objects (process/driver/ect.)
    // are used.
    FPGA *gDevice = nullptr;

    FPGA *GetDefaultFPGA() {
        if (!gDevice) {
            gDevice = new FPGA();
            assert(gDevice);
        }
        return gDevice;
    }

    FPGA::FPGA() {
        static char *args[4] = {};
        args[0] = "";
        args[1] = "-device";
        args[2] = "fpga";

        this->vmm = VMMDLL_Initialize(3, args);
        if (!this->vmm)
            throw std::runtime_error("failed to initialize device");

        if (!VMMDLL_ConfigGet(this->vmm, LC_OPT_FPGA_FPGA_ID, &this->deviceID) ||
            !VMMDLL_ConfigGet(this->vmm, LC_OPT_FPGA_VERSION_MAJOR, &this->majorVer) ||
            !VMMDLL_ConfigGet(this->vmm, LC_OPT_FPGA_VERSION_MINOR, &this->minorVer))
            throw std::runtime_error("failed to get device information");
    }

    FPGA::~FPGA() {
        assert(this->vmm && "null vmm");

        VMMDLL_Close(this->vmm);
    }

    bool FPGA::DisableMasterAbort() const {
        assert(this->vmm && "null vmm");

        if (this->majorVer < 4 || (this->majorVer < 5 && this->minorVer < 7))
            return false; // must be version 4.7 or newer!

        LC_CONFIG LcConfig = {};
        LcConfig.dwVersion = LC_CONFIG_VERSION;
#ifdef _WIN32
        strcpy_s(LcConfig.szDevice, "existing");
#else
        strcpy(LcConfig.szDevice, "existing");
#endif

        // fetch already existing leechcore handle.
        HANDLE hLC = LcCreate(&LcConfig);
        if (!hLC) return false;

        // enable auto-clear of status register [master abort].
        BYTE dataIn[4] =  {0x10, 0x00, 0x10, 0x00};
        LcCommand(
                hLC,
                LC_CMD_FPGA_CFGREGPCIE_MARKWR | 0x002,
                4,
                dataIn,
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

    bool FPGA::GetProcessInfo(uint32_t pid, VMMDLL_PROCESS_INFORMATION &info) {
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
        if (!VMMDLL_PidList(this->vmm, nullptr, &cPIDs)) return results;

        // we use alloca to store tmp data on stack for perf
        pdwPIDs = (PDWORD) alloca(cPIDs * sizeof(DWORD));
        memset(pdwPIDs, 0, cPIDs * sizeof(DWORD));
        if (!VMMDLL_PidList(this->vmm, pdwPIDs, &cPIDs))
            return results;

        for (SIZE_T i = 0; i < cPIDs; i++) {
            DWORD dwPid = pdwPIDs[i];

            // pull info from process
            VMMDLL_PROCESS_INFORMATION info = {0};
            if (!this->GetProcessInfo(dwPid, info))
                continue;

            // case-insensitive name check
            std::string procName(info.szNameLong);
            if (std::equal(name.begin(), name.end(), procName.begin(), [](char a, char b) {
                return std::tolower(a) == std::tolower(b);
            })) {
                results.push_back(dwPid);
            }
        }

        return results;
    }

    uint64_t FPGA::getDeviceID() const {
        assert(this->vmm && "null vmm");

        return this->deviceID;
    }

    void FPGA::getVersion(uint64_t &major, uint64_t &minor) const {
        assert(this->vmm && "null vmm");

        major = this->majorVer;
        minor = this->minorVer;
    }

} // memstream