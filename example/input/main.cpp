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

std::string GetKeyName(unsigned int virtualKey)
{
    unsigned int scanCode = MapVirtualKey(virtualKey, MAPVK_VK_TO_VSC);

    // because MapVirtualKey strips the extended bit for some keys
    switch (virtualKey)
    {
        case VK_LEFT: case VK_UP: case VK_RIGHT: case VK_DOWN: // arrow keys
        case VK_PRIOR: case VK_NEXT: // page up and page down
        case VK_END: case VK_HOME:
        case VK_INSERT: case VK_DELETE:
        case VK_DIVIDE: // numpad slash
        case VK_NUMLOCK:
        {
            scanCode |= 0x100; // set extended bit
            break;
        }
        default:
            break;
    }

    char keyName[50];
    if (GetKeyNameText(scanCode << 16, keyName, sizeof(keyName)) != 0)
    {
        return keyName;
    }
    else
    {
        return "[Error]";
    }
}

void key_event(int vk, bool down) {
    std::string key = GetKeyName(vk);
    if(down) {
        std::cout << "DOWN: ";
    } else {
        std::cout << "UP: ";
    }
    std::cout << key << std::endl;
}

int main() {
    try {
        FPGA* fpga = GetDefaultFPGA();
        uint64_t maj, min, dev = 0;
        fpga->getVersion(maj, min);
        dev = fpga->getDeviceID();

        std::cout << "FPGA: Device #" << dev << " v" << maj << "." << min << std::endl;

        Input in;
        in.OnKeyStateChange(key_event);

        while(true) {
            // read input from windows kernel
            if(!in.Update()) {
                printf("failed to update?!\n");;
                continue;
            }

            // if SPACE is down, print mouse pos
            if(in.IsKeyDown(VK_SPACE)) {
                auto pos = in.GetCursorPos();
                printf("MOUSE: (%ld, %ld)\n", pos.x, pos.y);
            };

            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    } catch (std::exception& ex) {
        printf("except: %s", ex.what());
        return 1;
    }
}