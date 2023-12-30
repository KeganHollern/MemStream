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

#define VMM_READ_FLAGS \
            (VMMDLL_FLAG_NO_PREDICTIVE_READ | \
            VMMDLL_FLAG_NOCACHE | \
            VMMDLL_FLAG_NOPAGING | \
            VMMDLL_FLAG_NOCACHEPUT)

namespace memstream {

    class MEMSTREAM_API FPGA {
    public:
        // TODO: add a constructor which takes the device ID
        FPGA();

        virtual ~FPGA();

         bool DisableMasterAbort() const;

         std::vector<uint32_t> GetAllProcessesByName(const std::string &name);

         bool GetProcessInfo(uint32_t pid, VMMDLL_PROCESS_INFORMATION &info);


        VMM_HANDLE getVmm();
        uint64_t getDeviceID() const;
        void getVersion(uint64_t& major, uint64_t& minor) const;
    protected:
        VMM_HANDLE vmm;
        uint64_t deviceID = 0;
        uint64_t majorVer = 0;
        uint64_t minorVer = 0;
    };

    MEMSTREAM_API FPGA *GetDefaultFPGA();
} // memstream

#endif //MEMSTREAM_FPGA_H
