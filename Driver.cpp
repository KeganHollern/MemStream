//
// Created by Kegan Hollern on 12/23/23.
//
#include <string>
#include <cassert>
#include <vmmdll.h>
#include "FPGA.h"
#include "Process.h"

#include "Driver.h"

namespace memstream {
    Driver::Driver(FPGA* pFPGA, std::string name) {
        //TODO: actually use the last process and not the first one

        pCsrss = new Process(pFPGA, "csrss.exe");
    }
    Driver::~Driver() {
        delete pCsrss;
    }
} // memstream