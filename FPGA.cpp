//
// Created by Kegan Hollern on 12/23/23.
//
#include <cassert>
#include <vmmdll.h>

#include "FPGA.h"

namespace memstream {
    // this might be retarded
    //TODO: ? global singleton ?
    FPGA* gDevice = nullptr;

    FPGA::FPGA() {
        if(gDevice == nullptr) gDevice = this;

        char* args[4] = {};
        args[0] = "";
        args[1] = "-device";
        args[2] = "fpga";

        PLC_CONFIG_ERRORINFO err = nullptr;
        this->vmm = VMMDLL_InitializeEx(3, args, &err);

        assert(this->vmm && "failed to initialize FPGA");
        //TODO: throw exception on error
    }
    FPGA::~FPGA() {
        if(gDevice == this) gDevice = nullptr;
        assert(this->vmm && "deconstructing with invalid hVMM");
        VMMDLL_Close(this->vmm);
    }

    void FPGA::DisableMasterAbort() {
        //TODO:
    }

    VMM_HANDLE FPGA::getVmm() {
        return vmm;
    }


} // memstream