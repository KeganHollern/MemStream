//
// Created by Kegan Hollern on 12/23/23.
//

#ifndef MEMSTREAM_FPGA_H
#define MEMSTREAM_FPGA_H

#if defined(_WIN32)
#if defined(MEMSTREAM_EXPORTS)
#define MEMSTREAM_API __declspec(dllexport)
#else
#define MEMSTREAM_API __declspec(dllimport)
#endif
#else
#if __GNUC__ >= 4
#define MEMSTREAM_API __attribute__ ((visibility ("default")))
#else
#define MEMSTREAM_API
#endif
#endif

#include <cstdint>
#include <vector>
#include <string>

#include <vmmdll.h>

namespace memstream {
    class MEMSTREAM_API FPGA {
    public:
        // TODO: add a constructor which takes the device ID

        // Attach to the default FPGA device on the system.
        FPGA();
        virtual ~FPGA();

        // Enable Auto-Clear of Master Abort bit.
        bool DisableMasterAbort() const;
        
        uint32_t GetProcessByName(const std::string& name);
        // Get all PIDs of processes by name.
        std::vector<uint32_t> GetAllProcessesByName(const std::string &name);
        // Get process information.
        bool GetProcessInfo(uint32_t pid, VMMDLL_PROCESS_INFORMATION &info);

        // Acquire the underlying VMM handle.
        // No longer valid if FPGA is deconstructed.
        VMM_HANDLE getVmm();

        // Get the device ID.
        uint16_t getDeviceID() const;

        // Get the fpga ID.
        uint16_t getFpgaID() const;

        // Get the device configuration space.
        std::vector<uint8_t> getCfgSpace() const;

        // TRUE if the TINY algorithm is being used.
        bool getTinyAlg() const;

        // TRUE if the device should retry on error.
        bool getRetry() const;

        // Get the maximum payload size for this device.
        uint32_t getMaxSize() const;

        // Get the delay between reads.
        uint32_t getReadDelay() const;

        // Get the delay between writes.
        uint32_t getWriteDelay() const;

        // Get the device PCILEECH-FPGA version.
        void getVersion(uint64_t &major, uint64_t &minor) const;
    protected:
        VMM_HANDLE vmm;

        uint64_t majorVer = 0;      // LC_OPT_FPGA_VERSION_MAJOR
        uint64_t minorVer = 0;      // LC_OPT_FPGA_VERSION_MINOR
        uint64_t tiny = 0;          // LC_OPT_FPGA_ALGO_TINY
        uint64_t deviceID = 0;      // LC_OPT_FPGA_DEVICE_ID
        uint64_t fpgaID = 0;        // LC_OPT_FPGA_FPGA_ID
        uint64_t readDelay = 0;     // LC_OPT_FPGA_DELAY_READ
        uint64_t writeDelay = 0;    // LC_OPT_FPGA_DELAY_WRITE
        uint64_t maxSize = 0;       // LC_OPT_FPGA_MAX_SIZE_RX
        uint64_t retry = 0;         // LC_OPT_FPGA_RETRY_ON_ERROR

        uint8_t configSpace[0x1000] = {0};
    };

    MEMSTREAM_API FPGA *GetDefaultFPGA();
    MEMSTREAM_API void DebugVMM();
} // memstream

#endif //MEMSTREAM_FPGA_H
