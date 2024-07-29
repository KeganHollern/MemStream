#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <format>


#include <vmmdll.h>
#pragma comment(lib, "vmm")
#pragma comment(lib, "leechcore")

#define READ_FLAGS \
    VMMDLL_FLAG_ZEROPAD_ON_FAIL | \
    VMMDLL_FLAG_NO_PREDICTIVE_READ | \
    VMMDLL_FLAG_NOCACHE | \
    VMMDLL_FLAG_NOCACHEPUT

namespace memstream::v2
{
    class DeviceConfiguration {
    public:
        uint64_t majorVer = 0;             // LC_OPT_FPGA_VERSION_MAJOR
        uint64_t minorVer = 0;             // LC_OPT_FPGA_VERSION_MINOR
        uint64_t tiny = 0;                 // LC_OPT_FPGA_ALGO_TINY
        uint64_t deviceID = 0;             // LC_OPT_FPGA_DEVICE_ID
        uint64_t fpgaID = 0;               // LC_OPT_FPGA_FPGA_ID
        uint64_t readDelay = 0;            // LC_OPT_FPGA_DELAY_READ
        uint64_t writeDelay = 0;           // LC_OPT_FPGA_DELAY_WRITE
        uint64_t maxSize = 0;              // LC_OPT_FPGA_MAX_SIZE_RX
        uint64_t retry = 0;                // LC_OPT_FPGA_RETRY_ON_ERROR
        uint8_t configSpace[0x1000] = {0}; // configspace

        inline bool CanAutoClearMasterAbort() {
             return !(this->majorVer < 4 || (this->majorVer < 5 && this->minorVer < 7));
        }

        inline std::string to_string() {
            return std::format("v{}.{} [TINY: {}] [ID: {}/{}] [READ_DELAY: {}] [WRITE_DELAY: {}] [MAX_SIZE: {}] [RETRY: {}]", 
                majorVer, minorVer, tiny, deviceID, fpgaID, readDelay, writeDelay, maxSize, retry);
        }
    };


    class FPGA {
    private:
        DeviceConfiguration _config;
        bool _free_handle = false;
        VMM_HANDLE _vmm = nullptr;

        void init();
        void readConfig();
        void disableMasterAbort();

    public:
        FPGA();
        virtual ~FPGA();

        // this allows us to init v2 on
        // top of a v2 fpga class
        FPGA(VMM_HANDLE vmm);

        // internal getters
        inline VMM_HANDLE getVMM() { return _vmm; }
        inline DeviceConfiguration* getConfiguration() { return &_config; }

        // utility
        uint32_t GetProcessId(const std::string& processName);
        std::vector<uint32_t> GetAllProcessIds(const std::string& processName);

        // reading
        void Read(uint32_t pid, uint64_t address, uint8_t* buffer, size_t size);
        template <typename T>
        void Read(uint32_t pid, uint64_t address, T& data) {
            this->Read(pid, address, reinterpret_cast<uint8_t*>(&data), sizeof(T));
        }

        // writing   
        void Write(uint32_t pid, uint64_t address, uint8_t* buffer, size_t size);
        template <typename T>
        void Write(uint32_t pid, uint64_t address, T& data) {
            this->Write(pid, address, reinterpret_cast<uint8_t*>(&data), sizeof(T));
        }

        uint64_t GetProcAddress(uint32_t pid, const std::string& moduleName, const std::string& procName);
        
        // TODO: replace the structs below and mem free inside them so all data is safely
        //       copied to our non-vmm controlled heap objects...

        // CALLER FREE: free
        PVMMDLL_PROCESS_INFORMATION GetProcessInfo(uint32_t pid);

        // CALLER FREE: VMMDLL_MemFree
        PVMMDLL_MAP_MODULEENTRY GetModuleInfo(uint32_t pid, const std::string& moduleName = "");

        // CALLER FREE: VMMDLL_MemFree
        PVMMDLL_MAP_MODULE GetModules(uint32_t pid);


        // Use REG_XXX macros from Windows.h !
        // CALLER FREE: free
        uint8_t* QueryWindowsRegistry(const std::string& path, uint32_t valueType);
        
    };
}   