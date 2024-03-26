#include <vector>
#include <cstring>
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <iostream>

#include <vmmdll.h>

#include "MemStream/FPGA.h"

namespace memstream {
    bool gDebug = false;
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

    void DebugVMM() {
        if(gDevice)
            throw std::runtime_error("cannot enable debugging after connecting to fpga device");

        gDebug = true;
    }
    
    FPGA::FPGA() {
        std::cout << "initializing fpga device..." << std::endl;
        LPCSTR args[] = {
            const_cast<LPCSTR>(""), 
            const_cast<LPCSTR>("-device"), 
            const_cast<LPCSTR>("fpga://algo=0"), 
            const_cast<LPCSTR>(""), 
            const_cast<LPCSTR>("")
        };
        int argc = 3;
        if(gDebug) {
            args[argc++] = const_cast<LPCSTR>("-v");
            args[argc++] = const_cast<LPCSTR>("-printf");
        }

        PLC_CONFIG_ERRORINFO errInfo = nullptr;
        this->vmm = VMMDLL_InitializeEx(argc, args, &errInfo);
        if (!this->vmm) {
            if(errInfo && errInfo->dwVersion == LC_CONFIG_ERRORINFO_VERSION) {
                std::wstring errmsg(errInfo->wszUserText, errInfo->cwszUserText);
                std::string asStr(errmsg.begin(), errmsg.end());
                std::cout << "VMM Error: " << asStr << std::endl;
            }
            std::cout << "errInfo ptr: " << errInfo << std::endl;

            throw std::runtime_error("VMMDLL_InitializeEx failed");
        }
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

    uint32_t FPGA::GetProcessByName(const std::string& name) {
        uint32_t pid = 0;
        VMMDLL_PidGetFromName(this->getVmm(), (LPSTR)name.c_str(), (PDWORD)&pid);
        return pid;
    }

    std::vector<uint32_t> FPGA::GetAllProcessesByName(const std::string &name) {

        PVMMDLL_PROCESS_INFORMATION process_info = NULL;
        DWORD total_processes = 0;
        std::vector<uint32_t> list = { };

        if (!VMMDLL_ProcessGetInformationAll(this->getVmm(), &process_info, &total_processes))
        {
            return list;
        }

        for (size_t i = 0; i < total_processes; i++)
        {
            auto process = process_info[i];
            if (strstr(process.szNameLong, name.c_str()))
                list.emplace_back(process.dwPID);
        }

        return list;
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