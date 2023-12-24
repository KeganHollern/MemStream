//
// Created by Kegan Hollern on 12/23/23.
//

#ifndef MEMSTREAM_FPGA_H
#define MEMSTREAM_FPGA_H

namespace memstream {

    uint64_t VMM_READ_FLAGS =
            VMMDLL_FLAG_NO_PREDICTIVE_READ |
            VMMDLL_FLAG_NOCACHE |
            VMMDLL_FLAG_NOPAGING |
            VMMDLL_FLAG_NOCACHEPUT;

    class FPGA {
    public:
        FPGA();

        virtual ~FPGA();

        virtual void DisableMasterAbort();

        VMM_HANDLE getVmm();

    private:
        VMM_HANDLE vmm;
    };

} // memstream

#endif //MEMSTREAM_FPGA_H
