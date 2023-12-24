//
// Created by Kegan Hollern on 12/23/23.
//

#ifndef MEMSTREAM_DRIVER_H
#define MEMSTREAM_DRIVER_H

#include "Process.h"

namespace memstream {

    //TODO: this may be the wrong approach...
    //TODO: make process a child of driver I guess (because `is64bit` isn't useful here and nore is PID)
    class Driver : Process {
    public:
        Driver(const std::string& name);
        Driver(FPGA *pFPGA, std::string name);

        virtual ~Driver();

        //TODO: base everyhting off the DRIVER module from csrss or something
    private:
        const std::string name;
    };

} // memstream

#endif //MEMSTREAM_DRIVER_H
