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

    private:
        FPGA *pFPGA;
    };

}

#endif //MEMSTREAM_KERNEL_H
