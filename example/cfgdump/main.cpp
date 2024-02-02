#include <cstdio>
#include <string>
#include <MemStream/FPGA.h>
#include <MemStream/Utils.h>

using namespace memstream;

int main() {
    try {
        FPGA* device = GetDefaultFPGA();
        std::vector<uint8_t> cfgSpace = device->getCfgSpace();
        log::buffer(&cfgSpace[0], 0x1000);
    } catch (std::exception& ex) {
        printf("except: %s", ex.what());
        return 1;
    }
}