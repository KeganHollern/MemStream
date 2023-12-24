//
// Created by Kegan Hollern on 12/23/23.
//
#include <cstdint>
#include <cassert>
#include <string>
#include <vmmdll.h>

#include "FPGA.h"
#include "Process.h"

namespace memstream {
    extern FPGA *gDevice;

    Process::Process(uint32_t pid) : Process(gDevice, pid) {}

    Process::Process(FPGA *pFPGA, uint32_t pid) : info() {
        //TODO: get process by PID / validate it exists?
        // maybe pull some basic proc details like Is64Bit?

        bool ok = pFPGA->GetProcessInfo(pid, this->info);
        //TODO: exception if error
        assert(ok && "failed to get process info");

        this->pFPGA = pFPGA;
    }

    Process::Process(const std::string &name) : Process(gDevice, name) {}

    Process::Process(FPGA *pFPGA, const std::string &name) : info() {
        uint32_t foundPid = 0;

        //TODO: find process by name
        bool ok = VMMDLL_PidGetFromName(pFPGA->getVmm(), (char *) name.c_str(), &foundPid);
        //TODO: throw exception if process not found
        assert(ok && "vmmdll failed to get pid from name");
        assert(foundPid && "pid result was 0 with ok feedback from vmm");

        ok = pFPGA->GetProcessInfo(foundPid, this->info);
        //TODO: exception if error
        assert(ok && "failed to get process info");

        this->pFPGA = pFPGA;
    }

    Process::~Process() = default;

    bool Process::isIs64Bit() const {
        return this->info.tpMemoryModel == VMMDLL_MEMORYMODEL_X64;
    }

    uint32_t Process::getPid() const {
        return this->info.dwPID;
    }

    bool Process::Read(uint64_t addr, void *buffer, uint32_t size) {
        return false;
    }

    bool Process::Write(uint64_t addr, void *buffer, uint32_t size) {
        return false;
    }

    uint64_t Process::FindPattern(uint64_t start, uint64_t stop, uint8_t *pattern, uint8_t *mask) {
        return 0;
    }

    uint64_t Process::GetModuleBase(const std::string &name) {
        return VMMDLL_ProcessGetModuleBaseU(
                this->pFPGA->getVmm(),
                this->getPid(),
                (char *) name.c_str());
    }

    bool Process::GetModuleInfo(const std::string &name, VMMDLL_MAP_MODULEENTRY &info) {

        // reading will alloc a ptr we need to free
        PVMMDLL_MAP_MODULEENTRY pModuleMapEntry;
        bool ok = VMMDLL_Map_GetModuleFromNameU(
                this->pFPGA->getVmm(),
                this->getPid(),
                (char*)name.c_str(),
                &pModuleMapEntry,
                NULL);
        if(!ok) return false;

        // copy ptr data to input struct & give result
        if(!std::memcpy(&info, pModuleMapEntry, sizeof(VMMDLL_MAP_MODULEENTRY)))
        {
            VMMDLL_MemFree(pModuleMapEntry);
            return false;
        }

        VMMDLL_MemFree(pModuleMapEntry);
        return true;
    }

    std::vector<VMMDLL_MAP_MODULEENTRY> Process::GetModules() {
        std::vector<VMMDLL_MAP_MODULEENTRY> results;

        PVMMDLL_MAP_MODULE pModuleMap = nullptr;

        if(!VMMDLL_Map_GetModuleU(this->pFPGA->getVmm(), this->getPid(), &pModuleMap, 0))
            return results;

        if(pModuleMap->dwVersion != VMMDLL_MAP_MODULE_VERSION) {
            VMMDLL_MemFree(pModuleMap);
            return results;
        }

        PVMMDLL_MAP_MODULEENTRY pModuleEntry;
        for(int i = 0; i < pModuleMap->cMap;i++) {
            pModuleEntry = pModuleMap->pMap + i;
            if(!pModuleEntry) continue;

            results.push_back(*pModuleEntry);
        }

        VMMDLL_MemFree(pModuleMap);
        return results;
    }

} // memstream