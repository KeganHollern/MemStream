//
// Created by Kegan Hollern on 12/23/23.
//
#include <string>
#include <cassert>
#include <utility>
#include <vmmdll.h>

#include "MemStream/FPGA.h"
#include "MemStream/Process.h"
#include "MemStream/Driver.h"

namespace memstream {
    // ex: Driver("win32kbase.sys")
    Driver::Driver(const std::string &name) : Driver(GetDefaultFPGA(), name) {}

    Driver::Driver(FPGA *pFPGA, const std::string &name) : info() {
        if (!pFPGA)
            throw std::invalid_argument("invalid fpga");

        auto pids = pFPGA->GetAllProcessesByName("csrss.exe");
        if (pids.empty())
            throw std::runtime_error("failed to find csrss.exe");

        uint32_t pid = *pids.end();
        this->proc = new Process(pFPGA, pid);

        if (!this->proc->GetModuleInfo(name, this->info))
            throw std::runtime_error("failed to find driver");
    }

    Driver::Driver(FPGA *pFPGA, uint32_t csrss, const std::string &name) : info() {
        if (!pFPGA)
            throw std::invalid_argument("invalid fpga");

        this->proc = new Process(pFPGA, csrss);

        if (!this->proc->GetModuleInfo(name, this->info))
            throw std::runtime_error("failed to find driver");
    }

    Driver::~Driver() {
        delete this->proc;
    }


} // memstream