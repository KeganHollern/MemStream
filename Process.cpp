//
// Created by Kegan Hollern on 12/23/23.
//
#include <cstdint>
#include <cassert>
#include <string>
#include <vmmdll.h>

#include "FPGA.h"

#include "Process.h"

namespace memstream {
    Process::Process(FPGA* pFPGA, uint64_t pid) {
        //TODO: get process by PID / validate it exists?
        // maybe pull some basic proc details like Is64Bit?

        this->is64bit = true;
        this->pid = pid;
    }
    Process::Process(FPGA* pFPGA, std::string name) {
        uint64_t pid = 0;

        //TODO: find process by name
        bool ok = VMMDLL_PidGetFromName(pFPGA->getVmm(), name.c_str(), &pid);
        //TODO: throw exception if process not found
        assert(ok && "vmdll failed to get pid from name");
        assert(pid && "pid result was 0 with ok feedback from vmm");

        Process(pFPGA, pid);
    }
    Process::~Process() {

    }

    bool Process::isIs64Bit() const {
        return is64bit;
    }

    uint64_t Process::getPid() const {
        return pid;
    }
} // memstream