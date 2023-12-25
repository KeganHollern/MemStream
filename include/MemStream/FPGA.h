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

#include <vmmdll.h>

namespace memstream {

    uint64_t VMM_READ_FLAGS =
            VMMDLL_FLAG_NO_PREDICTIVE_READ |
            VMMDLL_FLAG_NOCACHE |
            VMMDLL_FLAG_NOPAGING |
            VMMDLL_FLAG_NOCACHEPUT;

    class MEMSTREAM_API FPGA {
    public:
        // TODO: add a constructor which takes the device ID
        FPGA();

        virtual ~FPGA();

        virtual bool DisableMasterAbort();

        virtual std::vector<uint32_t> GetAllProcessesByName(const std::string &name);

        virtual bool GetProcessInfo(uint32_t pid, VMMDLL_PROCESS_INFORMATION &info);


        VMM_HANDLE getVmm();

    private:
        VMM_HANDLE vmm;
    };

    MEMSTREAM_API FPGA *GetDefaultFPGA();
} // memstream

#endif //MEMSTREAM_FPGA_H
