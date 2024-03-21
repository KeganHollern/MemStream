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
        }
        return gDevice;
    }

    FPGA::FPGA() {
        static LPCSTR args[4] = {};
        args[0] = "";
        args[1] = "-device";
        args[2] = "fpga";

        this->vmm = VMMDLL_Initialize(3, args);
        if (!this->vmm)
            throw std::runtime_error("failed to initialize device");

        // NOTE: only LC_OPT* can be acquired via VMMDLL_ConfigGet
        if (!VMMDLL_ConfigGet(this->vmm, LC_OPT_FPGA_FPGA_ID, &this->deviceID) ||
            !VMMDLL_ConfigGet(this->vmm, LC_OPT_FPGA_VERSION_MAJOR, &this->majorVer) ||
            !VMMDLL_ConfigGet(this->vmm, LC_OPT_FPGA_VERSION_MINOR, &this->minorVer))
            throw std::runtime_error("failed to get device information");


        // we don't really care if these succeed because nothing relies on it _internally_.
        // and we don't promise accuracy to the user for these.
        VMMDLL_ConfigGet(this->vmm, LC_OPT_FPGA_ALGO_TINY, &this->tiny);
        VMMDLL_ConfigGet(this->vmm, LC_OPT_FPGA_FPGA_ID, &this->fpgaID);
        VMMDLL_ConfigGet(this->vmm, LC_OPT_FPGA_DELAY_READ, &this->readDelay);
        VMMDLL_ConfigGet(this->vmm, LC_OPT_FPGA_DELAY_WRITE, &this->writeDelay);
        VMMDLL_ConfigGet(this->vmm, LC_OPT_FPGA_MAX_SIZE_RX, &this->maxSize);
        VMMDLL_ConfigGet(this->vmm, LC_OPT_FPGA_RETRY_ON_ERROR, &this->retry);


        // --- some leechcore internal things we need to call ---

        // send some initial LC commands to pull more data about the FPGA device
        LC_CONFIG LcConfig = {};
        LcConfig.dwVersion = LC_CONFIG_VERSION;
        strcpy_s(LcConfig.szDevice, "existing");

        // fetch already existing leechcore handle.
        HANDLE hLC = LcCreate(&LcConfig);
        if (!hLC)
            throw std::runtime_error("failed to create leechcore device");



        PBYTE cfgSpace = nullptr;
        DWORD cfgSpaceSize = 0; // should always out 0x1000 but 🤷‍♂️S
        LcCommand(
                hLC,
                LC_CMD_FPGA_PCIECFGSPACE,
                0,
                nullptr,
                &cfgSpace,
                &cfgSpaceSize);

        // if we got data out - copy it & free the heap allocation
        if(cfgSpace && cfgSpaceSize > 0) {
            memcpy(this->configSpace, cfgSpace, min(0x1000, cfgSpaceSize));
            LcMemFree(cfgSpace);
        } else {
            throw std::runtime_error("failed to query leechcore");
        }


        // close leechcore handle.
        LcClose(hLC);

    }

    FPGA::~FPGA() {
        // if we deconstruct the global default device, we need to ensure we nullptr it
        // this way getDefaultFpga() will still function!
        if(this == gDevice) {
            gDevice = nullptr;
        }
        VMMDLL_Close(this->vmm);
    }

    bool FPGA::DisableMasterAbort() const {
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

    bool FPGA::GetProcessInfo(uint32_t pid, VMMDLL_PROCESS_INFORMATION &info) {
        SIZE_T cbInfo = sizeof(VMMDLL_PROCESS_INFORMATION);
        info.magic = VMMDLL_PROCESS_INFORMATION_MAGIC;
        info.wVersion = VMMDLL_PROCESS_INFORMATION_VERSION;
        return VMMDLL_ProcessGetInformation(this->vmm, pid, &info, &cbInfo);
    }

    std::vector<uint32_t> FPGA::GetAllProcessesByName(const std::string &name) {
        std::vector<uint32_t> results;

        PDWORD pdwPIDs;
        SIZE_T cPIDs = 0;
        if (!VMMDLL_PidList(this->vmm, nullptr, &cPIDs))
            return results;

        pdwPIDs = new DWORD[cPIDs]();
        if (!VMMDLL_PidList(this->vmm, pdwPIDs, &cPIDs)) {
            delete[] pdwPIDs;
            return results;
        }

        for (SIZE_T i = 0; i < cPIDs; i++) {
            DWORD dwPid = pdwPIDs[i];

            // pull info from process
            VMMDLL_PROCESS_INFORMATION info = {0};
            if (!this->GetProcessInfo(dwPid, info))
                continue;

            // case-insensitive name check
            std::string procName(info.szNameLong);


            if (procName.size() == name.size() && std::equal(name.begin(), name.end(), procName.begin(), [](char a, char b) {
                return std::tolower(a) == std::tolower(b);
            })) {
                results.push_back(dwPid);
            }
        }

        delete[] pdwPIDs;
        return results;
    }


    VMM_HANDLE FPGA::getVmm() {
        return this->vmm;
    }

    std::vector<uint8_t> FPGA::getCfgSpace() const {
        return std::vector<uint8_t>(std::begin(this->configSpace), std::end(this->configSpace));
    }

    uint16_t FPGA::getDeviceID() const {
        return this->deviceID;
    }

    void FPGA::getVersion(uint64_t &major, uint64_t &minor) const {
        major = this->majorVer;
        minor = this->minorVer;
    }

    uint16_t FPGA::getFpgaID() const {
        return this->fpgaID;
    }

    bool FPGA::getTinyAlg() const {
        return this->tiny;
    }

    bool FPGA::getRetry() const {
        return this->retry;
    }

    uint32_t FPGA::getMaxSize() const {
        return this->maxSize;
    }

    uint32_t FPGA::getReadDelay() const {
        return this->readDelay;
    }

    uint32_t FPGA::getWriteDelay() const {
        return this->writeDelay;
    }

} // memstreamS