#include <stdexcept>

#include <vmmdll.h>

#include "MemStream/FPGA.h"
#include "MemStream/Windows/Kernel.h"

namespace memstream::windows {

    Kernel::Kernel(FPGA *pFPGA) {
        if (!pFPGA)
            throw std::runtime_error("null fpga");

        this->pFPGA = pFPGA;
    }

    Kernel::Kernel() : Kernel(GetDefaultFPGA()) {}

    Kernel::~Kernel() = default;

    // TODO: utils for kernel drivers

}