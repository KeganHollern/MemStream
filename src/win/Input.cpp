#include <string>
#include <cassert>
#include <stdexcept>
#include <cstring>

#include <vmmdll.h>

#include "MemStream/FPGA.h"
#include "MemStream/Process.h"
#include "MemStream/Windows/Input.h"


namespace memstream::windows {
    Input::Input() : Input(GetDefaultFPGA()) {}
    Input::Input(FPGA *pFPGA) {
        if (!pFPGA)
            throw std::invalid_argument("null fpga");

        this->kernel = getUserSessionKernelProcess(pFPGA);
        if(!this->kernel)
            throw std::runtime_error("could not find kernel session process");

        // windows 10 exports these...
        this->gafAsyncKeyStateAddr = this->kernel->GetExport("win32kbase.sys", "gafAsyncKeyState");
        this->gptCursorAsync = this->kernel->GetExport("win32kbase.sys", "gptCursorAsync");

        //Somtimes win32kbase screws up. Getting the import from win32kfull.sys is an alternative way
        if (!this->gafAsyncKeyStateAddr) {
            this->gafAsyncKeyStateAddr = this->kernel->GetImport("win32kfull.sys", "gafAsyncKeyState");
        }

        if (!this->gptCursorAsync) {
            this->gptCursorAsync = this->kernel->GetImport("win32kfull.sys", "gptCursorAsync");
        }

        if (!this->gafAsyncKeyStateAddr) {
            // probably windows 11 -- need to pull from win32ksgd.sys

            uint64_t base = this->kernel->GetModuleBase("win32ksgd.sys");

            if (!base)
                throw std::runtime_error("could not find win32ksgd.sys");

            uint64_t addr = base + 0x3110;
            uint64_t r1, r2, r3 = 0;

            if(!this->kernel->Read(addr, r1))
                throw std::runtime_error("failed to read win32ksgd.sys");

            if(!r1)
                throw std::runtime_error("failed to read r1 in win32ksgd.sys");

            for(int i = 0; i < 4; i++) {
                if(!this->kernel->Read(r1 + (i * 8), r2))
                    throw std::runtime_error("failed to read win32ksgd.sys");

                if(r2) break;
            }

            if(!r2)
                throw std::runtime_error("failed to read r2 in win32ksgd.sys");

            if(!this->kernel->Read(r2, r3))
                throw std::runtime_error("failed to read win32ksgd.sys");

            if(!r3)
                throw std::runtime_error("failed to read r3 in win32ksgd.sys");

            uint64_t result = r3 + 0x3690;

            this->gafAsyncKeyStateAddr = result;
        }

        if (!this->gafAsyncKeyStateAddr)
            throw std::runtime_error("failed to find gafAsyncKeyState");

        if (!this->gptCursorAsync)
            throw std::runtime_error("failed to find CURSOR");
    }

    Input::~Input() {
        assert(this->winlogon && "no winlogon process");
        delete this->kernel;
    }

    bool Input::Update() {
        assert(this->winlogon && "no winlogon process");
        assert(this->gafAsyncKeyStateAddr && "no keyboard addr");
        assert(this->gptCursorAsync && "no cursor addr");

        // save our old state
        std::memcpy(this->prevState, this->state, sizeof(uint8_t[64]));

        // scatter read these values
        std::vector<std::tuple<uint64_t, uint8_t *, uint32_t>> reads = {
                {this->gafAsyncKeyStateAddr, (uint8_t *) &this->state,     sizeof(uint8_t[64])},
                {this->gptCursorAsync,       (uint8_t *) &this->cursorPos, sizeof(MousePoint)},
        };

        if (!this->kernel->ReadMany(reads))
            return false;

        // at this point we have the current keyboard state in this->state and the last in this->prevState
        // so we can determine if any key input has changed
        // and handle callbacks for that as needed
        for (int vk = 0; vk < 256; ++vk) {
            if(this->key_callback) { // we want to capture key state changes
                if (this->IsKeyDown(vk) != this->WasKeyDown(vk)) { // the key state has changed since the last time we read
                    this->key_callback(vk, this->IsKeyDown(vk)); // notify of state change
                }
            }
        }

        return true;
    }

    bool Input::IsKeyDown(uint32_t vk) {
        return this->state[(vk * 2 / 8)] & 1 << vk % 4 * 2;
    }
    bool Input::WasKeyDown(uint32_t vk) {
        return this->prevState[(vk * 2 / 8)] & 1 << vk % 4 * 2;
    }

    void Input::OnKeyStateChange(void(*callback)(int, bool)) {
        this->key_callback = callback;
    }


    MousePoint Input::GetCursorPos() {
        return this->cursorPos;
    }

    Process *Input::GetKernelProcess() {
        return this->kernel;
    }

    Process* getUserSessionKernelProcess(FPGA *pFPGA) {
        uint32_t kernel_process_pid = 0;

        auto winlogon = pFPGA->GetAllProcessesByName("winlogon.exe");
        if(winlogon.size() == 1) {
            kernel_process_pid = winlogon[0];
        } else {
            // multiple winlogon procs... weird lets try csrss[1]
            auto csrss = pFPGA->GetAllProcessesByName("csrss.exe");
            if(csrss.size() >= 2) {
                kernel_process_pid = csrss[1];
            }
        }

        // unable to find kernel process...
        if(kernel_process_pid == 0)
            return nullptr;

        auto kernel = new Process(pFPGA, kernel_process_pid | VMMDLL_PID_PROCESS_WITH_KERNELMEMORY);
        return kernel;
    }
}