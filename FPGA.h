//
// Created by Kegan Hollern on 12/23/23.
//

#ifndef MEMSTREAM_FPGA_H
#define MEMSTREAM_FPGA_H

#include "api.h"

namespace memstream {

    uint64_t VMM_READ_FLAGS =
            VMMDLL_FLAG_NO_PREDICTIVE_READ |
            VMMDLL_FLAG_NOCACHE |
            VMMDLL_FLAG_NOPAGING |
            VMMDLL_FLAG_NOCACHEPUT;

    MEMSTREAM_API class FPGA {
    public:
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
