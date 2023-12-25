//
// Created by Kegan Hollern on 12/23/23.
//

#ifndef MEMSTREAM_DRIVER_H
#define MEMSTREAM_DRIVER_H

#include "api.h"

namespace memstream {

    class MEMSTREAM_API Driver {
    public:
        explicit Driver(const std::string &name);

        Driver(FPGA *pFPGA, const std::string &name);

        Driver(FPGA *pFPGA, uint32_t csrss, const std::string &name);

        virtual ~Driver();

    private:
        VMMDLL_MAP_MODULEENTRY info;
        Process *proc;
    };

} // memstream

#endif //MEMSTREAM_DRIVER_H
