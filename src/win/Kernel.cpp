#include <stdexcept>
#include <vector>
#include <cassert>

#include <vmmdll.h>

#include "MemStream/FPGA.h"
#include "MemStream/Windows/Kernel.h"

namespace memstream::windows {

    Kernel::Kernel(FPGA *pFPGA) {
        if (!pFPGA)
            throw std::runtime_error("null fpga");

        this->system = new Process(pFPGA, 4/* | VMMDLL_PID_PROCESS_WITH_KERNELMEMORY*/);
        this->pFPGA = pFPGA;


    }

    Kernel::Kernel() : Kernel(GetDefaultFPGA()) {}

    Kernel::~Kernel() {
        delete this->system;
    };

    std::vector<VMMDLL_MAP_SERVICEENTRY> Kernel::GetServices() {
        std::vector<VMMDLL_MAP_SERVICEENTRY> results;
        PVMMDLL_MAP_SERVICE pServices = nullptr;

        bool ok = VMMDLL_Map_GetServicesU(this->pFPGA->getVmm(), &pServices);

        if (!ok) return results;
        if (!pServices) return results;

        for (uint32_t i = 0; i < pServices->cMap; i++) {
            const auto &entry = pServices->pMap[i];
            results.emplace_back(entry);
        }

        VMMDLL_MemFree(pServices);
        return results;
    }

    // very simple example of dumping all `driver.sys` values out of
    std::vector<std::string> Kernel::GetLoadedDrivers() {
        assert(this->system && "null system");
        auto modules = this->system->GetModules();

        std::vector<std::string> results;
        for (auto &driver: modules) {
            results.emplace_back(driver.uszText);
        }

        return results;
    }
}