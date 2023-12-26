//
// Created by Kegan Hollern on 12/23/23.
//

#ifndef MEMSTREAM_KERNEL_H
#define MEMSTREAM_KERNEL_H

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

#include <cstdint>
#include <string>
#include <vector>

#include <vmmdll.h>

#include <MemStream/FPGA.h>
#include <MemStream/Process.h>

#include <MemStream/FPGA.h>

namespace memstream::windows {

    class MEMSTREAM_API Kernel {
    public:
        explicit Kernel(FPGA *pFPGA);

        Kernel();

        virtual ~Kernel();

        std::vector<std::string> GetLoadedDrivers();
        std::vector<VMMDLL_MAP_SERVICEENTRY> GetServices();
    private:
        FPGA *pFPGA;
        Process* system;
    };

}

#endif //MEMSTREAM_KERNEL_H
