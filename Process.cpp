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
    extern FPGA* gDevice;

    Process::Process(uint32_t pid) : Process(gDevice, pid) {}
    Process::Process(FPGA* pFPGA, uint32_t pid) {
        //TODO: get process by PID / validate it exists?
        // maybe pull some basic proc details like Is64Bit?

        this->pFPGA = pFPGA;
        this->is64bit = true;
        this->pid = pid;
    }
    Process::Process(const std::string& name) : Process(gDevice, name) {}
    Process::Process(FPGA* pFPGA, const std::string& name) {
        uint32_t foundPid = 0;

        //TODO: find process by name
        bool ok = VMMDLL_PidGetFromName(pFPGA->getVmm(), (char*)name.c_str(), &foundPid);
        //TODO: throw exception if process not found
        assert(ok && "vmmdll failed to get pid from name");
        assert(foundPid && "pid result was 0 with ok feedback from vmm");

        this->pFPGA = pFPGA;
        this->is64bit = true;
        this->pid = foundPid;
    }
    Process::~Process() = default;

    bool Process::isIs64Bit() const {
        return is64bit;
    }

    uint32_t Process::getPid() const {
        return pid;
    }

    bool Process::Read(uint64_t addr, void *buffer, uint32_t size) {
        return false;
    }

    bool Process::Write(uint64_t addr, void *buffer, uint32_t size) {
        return false;
    }

    uint64_t Process::FindPattern(uint64_t start, uint64_t stop, uint8_t *pattern, uint8_t *mask) {
        return 0;
    }

} // memstream