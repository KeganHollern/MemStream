#include <string>
#include <cassert>
#include <stdexcept>
#include <cstring>

#include <vmmdll.h>

#include "MemStream/FPGA.h"
#include "MemStream/Process.h"
#include "MemStream/Windows/Registry.h"
#include "MemStream/Windows/Input.h"


namespace memstream::windows {
    // forward declares of helpers...
    uint64_t getAsyncKeystateWin11(FPGA *pFPGA);
    uint64_t getAsyncKeystateWin10(FPGA *pFPGA);
    uint32_t getWindowsVersion(FPGA *pFPGA);
    uint64_t getAsyncCursor(FPGA *pFPGA);

    Input::Input() : Input(GetDefaultFPGA()) {}

    Input::Input(FPGA *pFPGA) {
        if (!pFPGA)
            throw std::invalid_argument("null fpga");

        uint32_t version = getWindowsVersion(pFPGA);
        //TODO: fix
        // some errors on win10 cause this shit to b wrong ?!
        //      if(version == 0)
        //          throw std::runtime_error("failed to detect windows version");

        // --- create winlogon process w/ kernel memory access :)

        this->winlogon = getUserSessionKernelProcess(pFPGA);
        if(!this->winlogon)
            throw std::runtime_error("could not find kernel session process");


        // depending on win version (10 vs 11) grab keyboard state...
        if (version > 22000) {
            uint64_t base = this->winlogon->GetModuleBase("win32ksgd.sys");

            if (!base)
                throw std::runtime_error("could not find win32ksgd.sys");

            uint64_t addr = base + 0x3110;
            uint64_t r1, r2, r3 = 0;

            if(!this->winlogon->Read(addr, r1))
                throw std::runtime_error("failed to read win32ksgd.sys");

            if(!r1)
                throw std::runtime_error("failed to read r1 in win32ksgd.sys");

            for(int i = 0; i < 4; i++) {
                if(!this->winlogon->Read(r1 + (i * 8), r2))
                    throw std::runtime_error("failed to read win32ksgd.sys");

                if(r2) break;
            }

            if(!r2)
                throw std::runtime_error("failed to read r2 in win32ksgd.sys");

            if(!this->winlogon->Read(r2, r3))
                throw std::runtime_error("failed to read win32ksgd.sys");

            if(!r3)
                throw std::runtime_error("failed to read r3 in win32ksgd.sys");

            uint64_t result = r3 + 0x3690;

            this->gafAsyncKeyStateAddr = result;
        } else {
            this->gafAsyncKeyStateAddr = this->winlogon->GetExport("win32kbase.sys", "gafAsyncKeyState");
        }

        this->gptCursorAsync = this->winlogon->GetExport("win32kbase.sys", "gptCursorAsync");

        if (this->gafAsyncKeyStateAddr <= 0x7FFFFFFFFFFF)
            throw std::runtime_error("failed to find gafAsyncKeyState");

        if (this->gptCursorAsync <= 0x7FFFFFFFFFFF)
            throw std::runtime_error("failed to find CURSOR");



    }

    Input::~Input() {
        assert(this->winlogon && "no winlogon process");
        delete this->winlogon;
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

        if (!this->winlogon->ReadMany(reads))
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
        return this->winlogon;
    }

    //--- util functions for kernel interaction....

    uint32_t getWindowsVersion(FPGA *pFPGA) {
        if (!pFPGA) return 0;

        // rip target version from registry
        std::wstring version;
        Registry reg(pFPGA);
        bool ok = reg.Query(
                "HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\CurrentBuild",
                RegistryType::sz,
                version);
        if (!ok) return 0;

        return std::stoi(version);
    }

    Process *windows::getUserSessionKernelProcess(FPGA *pFPGA) {
        auto pids = pFPGA->GetAllProcessesByName("winlogon.exe");
        auto backup_procs = pFPGA->GetAllProcessesByName("csrss.exe");

        pids.insert(pids.end(), backup_procs.begin(), backup_procs.end());

        bool win11 = getWindowsVersion(pFPGA) > 22000;

        Process* result = nullptr;
        for(const auto& pid : pids) {
            result = new Process(pFPGA, pid | VMMDLL_PID_PROCESS_WITH_KERNELMEMORY);

            // try to find cursor async...
            if(result->GetModuleBase("win32kbase.sys")) { // win10 and win11 need this
                if(!win11 || result->GetModuleBase("win32ksgd.sys")) { // if on win11 we need to find win32ksgd.sys
                    if (result->GetExport("win32kbase.sys", "gptCursorAsync")) { // we need to find this export...
                        return result;
                    }
                }
            }


            delete result;
            result = nullptr;
        }

        return nullptr;
    }
}