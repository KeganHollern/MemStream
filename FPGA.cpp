//
// Created by Kegan Hollern on 12/23/23.
//
#include <vmmdll.h>
#include <cassert>
#include <exception>

#include "FPGA.h"

namespace memstream {
    FPGA::FPGA() {
        char* args[4] = {};
        args[0] = "";
        args[1] = "-device";
        args[2] = "fpga";

        PLC_CONFIG_ERRORINFO err = 0;
        this->vmm = VMMDLL_InitializeEx(3, args, &err);

        assert(this->vmm && "failed to initialize FPGA");
        //TODO: throw exception on error
    }
    FPGA::~FPGA() {
        assert(this->vmm && "deconstructing with invalid hVMM");
        //TODO: vmmdll_close() ???

    }

    void FPGA::DisableMasterAbort() {
        //TODO:
    }

    const tdVMM_HANDLE *FPGA::getVmm() const {
        return vmm;
    }


} // memstream