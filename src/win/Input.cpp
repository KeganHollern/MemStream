#include <string>
#include <cassert>
#include <stdexcept>
#include <cstring>
#include <algorithm>
#include <iostream>

#include <vmmdll.h>

#include "MemStream/FPGA.h"
#include "MemStream/Windows/Registry.h"

#include "MemStream/Windows/Input.h"


namespace memstream::windows {

    // algorithm for pulling gafAsyncKeyState from win32ksg
    // on windows version >22000
    uint64_t win32ksgd(memstream::FPGA* pFPGA) {
        if(!pFPGA)
            throw std::runtime_error("null fpga in win32ksgd");

        // get all csrss
        auto procs = pFPGA->GetAllProcessesByName("csrss.exe");

        // not enough ?!
        if(procs.size() < 2)
            throw std::runtime_error("could not find min 2 csrss.exe applications");

        // sort them (because i don't trust ulf) and grab the 2nd process
        std::sort(procs.begin(), procs.end()); 
        uint32_t pid = procs.at(1);

        memstream::Process csrss(pFPGA, pid);
        csrss.setPaging(true); // ensure paged data is read

        uint64_t base = csrss.GetModuleBase("win32ksgd.sys");
        if(!base)
            throw std::exception("could not find win32ksgd.sys");

        uintptr_t addr = base + 0x3110;
        uint64_t r1, r2, r3 = 0;

        if(!csrss.Read(addr, r1))
            throw std::runtime_error("failed to read win32ksgd.sys");

        if(!r1)
            throw std::runtime_error("failed to read r1 in win32ksgd.sys");

        for(int i = 0; i < 4; i++) {
            if(!csrss.Read(r1 + (i * 8), r2))
                throw std::runtime_error("failed to read win32ksgd.sys");
            
            if(r2) break;
        }

        if(!r2)
            throw std::runtime_error("failed to read r2 in win32ksgd.sys");

        if(!csrss.Read(r2, r3))
            throw std::runtime_error("failed to read win32ksgd.sys");

        if(!r3)
            throw std::runtime_error("failed to read r3 in win32ksgd.sys");

        uint64_t result = r3 + 0x3690;
        if (result <= 0x7FFFFFFFFFFF)
            throw std::runtime_error("could not find gafAsyncKeyState in win32ksgd.sys");

        return result;
    }
    uint64_t findkbaseexport(memstream::Process* kernel, const std::string& name) {
        uint64_t addr = kernel->GetExport("win32kbase.sys", name); // try to pull export from kbase
        if(addr <= 0x7FFFFFFFFFFF)
            addr = kernel->GetImport("win32kfull.sys", name); // try to pull import from kfull
        
        if(addr <= 0x7FFFFFFFFFFF)
            addr = 0;

        return addr;
    }


    Input::Input() : Input(GetDefaultFPGA()) {}
    Input::Input(FPGA *pFPGA) {
        if (!pFPGA)
            throw std::invalid_argument("null fpga");

        // yes all of this stuff was lifted straight from Metick's DMA library
        // my shit was broken so I copied him hoping it would fix it...
        std::string win = "";
        try {
	        win = QueryValue("HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\CurrentBuild", e_registry_type::sz, pFPGA);
        } catch (std::exception& e) {
            std::cout << "CRITICAL: failed registry query - is your firmware up to date? assuming windows 10" << std::endl;
        }

        int Winver = 0;
        if (!win.empty())
            Winver = std::stoi(win);


        // find winlogon and store it as our "kernel" process
        uint32_t pid = pFPGA->GetProcessByName("winlogon.exe");
        this->kernel = new memstream::Process(pFPGA, pid | VMMDLL_PID_PROCESS_WITH_KERNELMEMORY);
        this->kernel->setPaging(true); // ensure we read paged data...
        
        if (Winver > 22000) {
            this->gafAsyncKeyStateAddr = win32ksgd(pFPGA);
        } else {
            this->gafAsyncKeyStateAddr = findkbaseexport(this->kernel, "gafAsyncKeyState");
            if(!this->gafAsyncKeyStateAddr)
                throw std::runtime_error("unable to find gafAsyncKeyState import/export");
        }
        this->gptCursorAsync = findkbaseexport(this->kernel, "gptCursorAsync"); 
        if(!this->gptCursorAsync)
            throw std::runtime_error("unable to find gafAsyncKeyState import/export");
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
        this->prevPos = this->cursorPos;

        // scatter read these values
        std::list<std::shared_ptr<memstream::ScatterOp>> reads = {
            std::make_shared<memstream::ScatterOp>(this->gafAsyncKeyStateAddr, (uint8_t *) &this->state, sizeof(uint8_t[64])),
            std::make_shared<memstream::ScatterOp>(this->gptCursorAsync, (uint8_t *) &this->cursorPos, sizeof(MousePoint)),
        };

        if (!this->kernel->ReadMany(reads))
            return false;

        // at this point we have the current keyboard state in this->state and the last in this->prevState
        // so we can determine if any key input has changed
        // and handle callbacks for that as needed
        if(this->key_callback) { 
            for (int vk = 0; vk < 256; ++vk) {
                if (this->IsKeyDown(vk) != this->WasKeyDown(vk)) { // the key state has changed since the last time we read
                    this->key_callback(vk, this->IsKeyDown(vk)); // notify of state change
                }
            }
        }
            
        // we have the mouse state
        // so we want to call the mouse callback with
        // our change in X/Y pos, and the new target pos
        if(this->mouse_callback) {
            MousePoint deltaPos = {0};
            if(!first_update) {
                deltaPos.x = this->cursorPos.x - this->prevPos.x;
                deltaPos.y = this->cursorPos.y - this->prevPos.y;
            }

            this->mouse_callback(deltaPos, this->cursorPos);
        }

        first_update = false;
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
    void Input::OnMouseMove(void(*callback)(MousePoint,MousePoint)) {
        this->mouse_callback = callback;
    }


    MousePoint Input::GetCursorPos() {
        return this->cursorPos;
    }

    Process *Input::GetKernelProcess() {
        return this->kernel;
    }
}