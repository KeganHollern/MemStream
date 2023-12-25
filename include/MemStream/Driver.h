//
// Created by Kegan Hollern on 12/23/23.
//

#ifndef MEMSTREAM_DRIVER_H
#define MEMSTREAM_DRIVER_H

#if defined(_WIN32)
#if defined(MEMSTREAM_EXPORTS)
#define MEMSTREAM_API __declspec(dllexport)
#else
#define MEMSTREAM_API __declspec(dllimport)
#endif
#else
#if __GNUC__ >= 4
#define MEMSTREAM_API __attribute__ ((visibility ("default")))
#else
#define MEMSTREAM_API
#endif
#endif

#include <vmmdll.h>
#include <MemStream/FPGA.h>
#include <MemStream/Process.h>

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
