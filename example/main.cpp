#include <cstdio>
#include <string>
#include <cstdint>

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

        uint8_t value = 0;
        if(!proc.Read(base,&value, 1)) {
            printf("failed read");
            return 1;
        }

        printf("%x", value);

        return 0;
    } catch (std::exception& ex) {
        printf("except: %s", ex.what());
        return 1;
    }
}