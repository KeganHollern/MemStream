#pragma once

#include <Windows.h>

#include "FPGA.h"
#include "Process.h"

namespace memstream::v2
{
    typedef void(*fncOnKeyStateChange)(int, bool);
    typedef void(*fncOnMouseMove)(POINT, POINT);

    class Input {
    private:
        FPGA* _fpga;
            
        struct keyboard_state {
            uint8_t current[64];
            uint8_t previous[64];
            fncOnKeyStateChange callback;
        } _keyboard = {0};

        struct mouse_state {
            POINT current;
            POINT previous;
            fncOnMouseMove callback;
        } _mouse = {0};

        struct kernel {
            Process* process;
            uint64_t gafAsyncKeyStateAddr;
            uint64_t gptCursorAsync;
        } _kernel = {0};


        uint32_t getWindowsVersion();
    public:
        Input(FPGA* fpga);
        ~Input();

        // read keyboard & mouse state
        void Read(); 

        // use VIRTUAL_KEY_CODE to check if key is pressed
        bool IsKeyDown(uint32_t vk, bool once = false);
        // get current cursor position
        POINT GetCursorPos();

        // set a callback to be run on key state change
        //  Callback(VIRTUAL_KEY_CODE, IS_DOWN)
        void OnKeyStateChange(fncOnKeyStateChange callback); 
        // set a callback to be run on mouse pos change
        // Callback(CHANGE_IN_MOUSE_POSITION, NEW_MOUSE_POSITION)
        void OnMouseMove(fncOnMouseMove callback);

    };
}