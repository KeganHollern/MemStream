#include <cstdio>
#include <string>
#include <sstream>
#include <vector>
#include <tuple>

#include <MemStream/FPGA.h>

using namespace memstream;

// this is a way of parsing patterns of bytes so we can
// look for them in the cfgspace
std::vector<std::tuple<uint8_t, bool>> parsePattern(const std::string &pattern) {
    std::vector<std::tuple<uint8_t, bool>> result;

    std::istringstream iss(pattern);
    std::string hexStr;

    while (iss >> hexStr) {
        if (hexStr.empty()) continue;
        if (hexStr == "?" || hexStr == "??" || hexStr == "XX") {
            result.emplace_back(0, false);
        } else {
            auto byte = static_cast<uint8_t>(std::stoul(hexStr, nullptr, 16));
            result.emplace_back(byte, true);
        }
    }

    return result;
}



bool Block60Check(uint8_t* pcfg) {
    auto search = parsePattern("10 00 02 00 e2 8f XX XX XX XX XX XX 12 f4 03 00");
    for(int i = 0; i < search.size(); i++) {
        if(!std::get<1>(search[i])) continue;
        if(pcfg[0x60] != std::get<0>(search[i])) return false;
    }
    return true;
}

bool Block40Check(uint8_t* pcfg) {
    auto search = parsePattern("01 48 03 78 08 00 00 00 05 60 80 00 00 00 00 00");
    for(int i = 0; i < search.size(); i++) {
        if(!std::get<1>(search[i])) continue;
        if(pcfg[0x40] != std::get<0>(search[i])) return false;
    }
    return true;
}



int main() {
    try {
        FPGA* device = GetDefaultFPGA();
        std::vector<uint8_t> cfgSpace = device->getCfgSpace();

        uint8_t* pcfg = &cfgSpace[0]; // this is 0x1000 in length!

        bool detected = false;
        if(Block40Check(pcfg)) {
            printf("DETECTED BATTLEYE BLOCK 40\n");
            detected = true;
        }

        if(Block60Check(pcfg)) {
            printf("DETECTED BATTLEYE BLOCK 60\n");
            detected = true;
        }

        // TODO: DEVICE ID CHECK FOR STOCK FIRMWARE

        // TODO: add more checks as we figure them out

        if(detected) {
            exit(1);
        }

        printf("maybe clean- no detections found :)\n");
    } catch (std::exception& ex) {
        printf("except: %s", ex.what());
        return 1;
    }
}

