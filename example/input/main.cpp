#include <cstdio>
#include <string>
#include <cstdint>
#include <chrono>
#include <thread>

#include <MemStream/Process.h>
#include <MemStream/Windows/Input.h>

using namespace memstream::windows;

int main() {
    try {
        Input in;

        while(true) {
            // read input from windows kernel
            if(!in.Update()) {
                printf("failed to update?!\n");;
                continue;
            }

            // print cursor pos
            auto pos = in.GetCursorPos();
            printf("MOUSE: (%l, %l)\n", pos.x, pos.y);

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