//
// Created by Kegan Hollern on 12/23/23.
//
#include <string>
#include <cassert>
#include <utility>
#include <vmmdll.h>
#include "FPGA.h"
#include "Process.h"

#include "Driver.h"

namespace memstream {
    extern FPGA* gDevice;

    Driver::Driver(const std::string &name) : Driver(gDevice, name) {}
    Driver::Driver(FPGA* pFPGA, std::string name) : Process(pFPGA, "csrss.exe"), name(std::move(name)) {}
    Driver::~Driver() = default;
} // memstream