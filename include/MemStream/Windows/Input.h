//
// Created by Kegan Hollern on 12/23/23.
//

#ifndef MEMSTREAM_INPUT_H
#define MEMSTREAM_INPUT_H

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

namespace memstream::windows {

    typedef struct MousePoint {
        long x;
        long y;
    } MousePoint;

    class MEMSTREAM_API Input {
    public:
        explicit Input(FPGA *pFPGA);

        Input();

        virtual ~Input();

        bool Update();

        bool IsKeyDown(uint32_t vk);

        MousePoint GetCursorPos();

    private:
        Process *winlogon;
        // kernel virtual address of async keystate value
        uint64_t gafAsyncKeyStateAddr;
        uint64_t gptCursorAsync;

        MousePoint cursorPos{0};
        uint8_t state[64]{0};
        uint8_t prevState[256 / 8]{0};
    };

}

#endif //MEMSTREAM_INPUT_H
