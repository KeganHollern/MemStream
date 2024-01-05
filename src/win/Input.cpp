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

    uint64_t getAsyncCursorWin11(FPGA *pFPGA);

    uint64_t getAsyncCursorWin10(FPGA *pFPGA);

    Input::Input() : Input(GetDefaultFPGA()) {}

    Input::Input(FPGA *pFPGA) {
        if (!pFPGA)
            throw std::invalid_argument("null fpga");

        DWORD pid = 0;
        if (!VMMDLL_PidGetFromName(pFPGA->getVmm(), "winlogon.exe", &pid))
            throw std::runtime_error("failed to find winlogon");

        uint32_t version = getWindowsVersion(pFPGA);
        if(version == 0)
            throw std::runtime_error("failed to detect windows version");

        // find kernel locations of our data
        if (version > 22000) {
            this->gafAsyncKeyStateAddr = getAsyncKeystateWin11(pFPGA);
            this->gptCursorAsync = getAsyncCursorWin11(pFPGA);
        } else {
            this->gafAsyncKeyStateAddr = getAsyncKeystateWin10(pFPGA);
            this->gptCursorAsync = getAsyncCursorWin10(pFPGA);
        }

        // failed to find :(
        if (this->gafAsyncKeyStateAddr <= 0x7FFFFFFFFFFF)
            throw std::runtime_error("failed to find gafAsyncKeyState");

        if (this->gptCursorAsync <= 0x7FFFFFFFFFFF)
            throw std::runtime_error("failed to find CURSOR");

        this->winlogon = new Process(pFPGA, pid | VMMDLL_PID_PROCESS_WITH_KERNELMEMORY);
    }

    Input::~Input() {
        assert(this->winlogon && "no winlogon process");

        delete this->winlogon;
    }

    bool Input::Update() {
        assert(this->winlogon && "no winlogon process");
        assert(this->gafAsyncKeyStateAddr && "no keyboard addr");
        assert(this->gptCursorAsync && "no cursor addr");

        uint8_t previous_key_state_bitmap[64] = {0};
        std::memcpy(previous_key_state_bitmap, this->state, sizeof(uint8_t[64]));

        /* Example of staged reads
        this->winlogon->StageRead(this->gafAsyncKeyStateAddr, this->state);
        this->winlogon->StageRead(this->gptCursorAsync, this->cursorPos);
        if(!this->winlogon->ExecuteStagedReads())
            return false;
        */

        // using scatter read to optimize
        std::vector<std::tuple<uint64_t, uint8_t *, uint32_t>> reads = {
                {this->gafAsyncKeyStateAddr, (uint8_t *) &this->state,     sizeof(uint8_t[64])},
                {this->gptCursorAsync,       (uint8_t *) &this->cursorPos, sizeof(MousePoint)},
        };

        if (!this->winlogon->ReadMany(reads))
            return false;

        for (int vk = 0; vk < 256; ++vk) {
            if ((this->state[((vk * 2) / 8)] & 1 << (vk % 8)) &&
                !(previous_key_state_bitmap[((vk * 2) / 8)] & 1 << (vk % 8)))
                this->prevState[vk / 8] |= 1 << (vk % 8);

            if(!this->key_callback) continue;

            if(this->OnPress(vk)) key_callback(vk, true);
            if(this->OnRelease(vk)) key_callback(vk, false);
        }



        return true;
    }

    bool Input::IsKeyDown(uint32_t vk) {
        return this->state[((vk * 2) / 8)] & 1 << (vk % 8);
    }
    bool Input::WasKeyDown(uint32_t vk) {
        return this->prevState[((vk * 2) / 8)] & 1 << (vk % 8);
    }

    bool Input::OnPress(uint32_t vk) {
        return this->IsKeyDown(vk) && !this->WasKeyDown(vk);
    }
    bool Input::OnRelease(uint32_t vk) {
        return this->WasKeyDown(vk) && !this->IsKeyDown(vk);
    }

    void Input::OnKeyStateChange(void(*callback)(int, bool)) {

    }


    MousePoint Input::GetCursorPos() {
        return this->cursorPos;
    }

    //--- util functions for kernel interaction....

    uint32_t getWindowsVersion(FPGA *pFPGA) {
        if (!pFPGA) return 0;

        // rip target version from registry
        std::wstring version;
        Registry reg(pFPGA);
        bool ok = reg.Query(
                R"(HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\CurrentBuild)",
                RegistryType::sz,
                version);
        if (!ok) return 0;

        return std::stoi(version);
    }

    uint64_t getAsyncCursorWin10(FPGA *pFPGA) {
        if (!pFPGA) return 0;

        auto pids = pFPGA->GetAllProcessesByName("csrss.exe");
        for(auto& pid : pids) {
            Process tmp(pFPGA, pid); // csrss is special and can access .sys modules without flag
            auto result = tmp.GetExport("win32kbase.sys", "gptCursorAsync");
            if(result > 0x7FFFFFFFFFFF)
                return result;
        }

        return 0;
    }

    uint64_t getAsyncKeystateWin10(FPGA *pFPGA) {
        if (!pFPGA) return 0;

        auto pids = pFPGA->GetAllProcessesByName("csrss.exe");
        for(auto& pid : pids) {
            Process tmp(pFPGA, pid); // csrss is special and can access .sys modules without flag
            auto result = tmp.GetExport("win32kbase.sys", "gafAsyncKeyState");
            if(result > 0x7FFFFFFFFFFF)
                return result;
        }

        return 0;
    }

    uint64_t getAsyncKeystateWin11(FPGA *pFPGA) {
        if (!pFPGA) return 0;

        // search for async keystate export addr
        auto pids = pFPGA->GetAllProcessesByName("csrss.exe");
        for(auto& pid : pids) {

            Process tmp(pFPGA, pid); // csrss is special and can access .sys modules without flag

            uint64_t base = tmp.GetModuleBase("win32ksgd.sys");
            if (!base) return 0;

            uint64_t addr = base + 0x3110;
            uint64_t r1, r2, r3 = 0;

            if(!tmp.Read(addr, r1)) return 0;
            if(!r1) return 0;

            if(!tmp.Read(r1, r2)) return 0;
            if(!r2) return 0;

            if(!tmp.Read(r2, r3)) return 0;
            if(!r3) return 0;

            uint64_t result = r3 + 0x3690;

            // this csrss process had it :)
            if (result > 0x7FFFFFFFFFFF)
                return result;
        }

        return 0;
    }

    uint64_t getAsyncCursorWin11(FPGA *pFPGA) {
        // no change for Win11- can use same routine as Win10
        return getAsyncCursorWin10(pFPGA);
    }
}