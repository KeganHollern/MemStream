#include <cstdio>
#include <string>
#include <cstdint>
#include <chrono>
#include <thread>
#include <iostream>

#include <MemStream/FPGA.h>
#include <MemStream/Process.h>
#include <MemStream/Utils.h>
#include <MemStream/Windows/Input.h>
#include <MemStream/Windows/Registry.h>

using namespace memstream;
using namespace memstream::windows;

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

    std::wcout << L"Win Ver: " << version << std::endl;

    return std::stoi(version);
}

int main() {
    try {
        FPGA* fpga = GetDefaultFPGA();
        uint64_t maj, min, dev = 0;
        fpga->getVersion(maj, min);
        dev = fpga->getDeviceID();

        std::cout << "FPGA: Device #" << dev << " v" << maj << "." << min << std::endl;

        Input in;

        while(true) {
            // read input from windows kernel
            if(!in.Update()) {
                printf("failed to update?!\n");;
                continue;
            }

            // print cursor pos
            auto pos = in.GetCursorPos();
            printf("MOUSE: (%ld, %ld)\n", pos.x, pos.y);

            // check if VK_SPACE is down (exit if true)
            if(in.IsKeyDown(0x20)) {
                printf("SPACE!");
                return 0;
            };

            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    } catch (std::exception& ex) {
        printf("except: %s", ex.what());
        return 1;
    }
}