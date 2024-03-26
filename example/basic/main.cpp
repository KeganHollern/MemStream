#include <cstdio>
#include <string>
#include <cstdint>
#include <chrono>
#include <string>
#include <MemStream/Process.h>

using namespace memstream;

int main() {
    try {
        Process proc("notepad.exe");

        uint64_t base = proc.GetModuleBase("notepad.exe");
        if(!base) {
            printf("failed getbase");
            return 1;
        }

        auto start = std::chrono::high_resolution_clock::now();
        uint8_t value = 0;
        if(!proc.Read(base,&value, 1)) {
            printf("failed read");
            return 1;
        }
        std::chrono::duration<double> diff = std::chrono::high_resolution_clock::now() - start;
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(diff);
        std::printf("%lld\n", ns.count());

        printf("%x", value);

        return 0;
    } catch (std::exception& ex) {
        printf("except: %s", ex.what());
        return 1;
    }
}