//
// Created by Kegan Hollern on 12/23/23.
//

#ifndef MEMSTREAM_DRIVER_H
#define MEMSTREAM_DRIVER_H

namespace memstream {

    class Driver {
    public:
        Driver(FPGA *pFPGA, std::string name);

        virtual ~Driver();

    private:
        Process* pCsrss;
    };

} // memstream

#endif //MEMSTREAM_DRIVER_H
