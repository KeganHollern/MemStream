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
    Process::Process(uint32_t pid) : Process(GetDefaultFPGA(), pid) {}

    Process::Process(FPGA *pFPGA, uint32_t pid) : info() {
        if(!pFPGA)
            throw std::invalid_argument("invalid fpga provided");

        if(!pFPGA->GetProcessInfo(pid, this->info))
            throw std::runtime_error("failed to get process info");

        this->pFPGA = pFPGA;
    }

    Process::Process(const std::string &name) : Process(GetDefaultFPGA(), name) {}

    Process::Process(FPGA *pFPGA, const std::string &name) : info() {
        if(!pFPGA)
            throw std::invalid_argument("invalid fpga provided");

        uint32_t foundPid = 0;
        if(!VMMDLL_PidGetFromName(pFPGA->getVmm(), (char *) name.c_str(), &foundPid))
            throw std::runtime_error("failed to find process with name");

        if(!pFPGA->GetProcessInfo(foundPid, this->info))
            throw std::runtime_error("failed to get process info");

        this->pFPGA = pFPGA;
    }

    Process::~Process() = default;

    bool Process::isIs64Bit() const {
        assert(this->pFPGA && "null fpga");

        return this->info.tpMemoryModel == VMMDLL_MEMORYMODEL_X64;
    }

    uint32_t Process::getPid() const {
        assert(this->pFPGA && "null fpga");

        return this->info.dwPID;
    }

    bool Process::Read(uint64_t addr, uint8_t *buffer, uint32_t size) {
        assert(this->pFPGA && "null fpga");
        assert(this->getPid() && "null pid");

        if(!addr) return false;
        if(!buffer) return false;
        if(!size) return false;

        return VMMDLL_MemReadEx(
                this->pFPGA->getVmm(),
                this->getPid(),
                addr,
                buffer,
                size,
                nullptr,
                VMM_READ_FLAGS);
    }

    bool Process::ReadMany(std::vector<std::tuple<uint64_t, uint8_t*, uint32_t>> &readOps) {
        assert(this->pFPGA && "null fpga");
        assert(this->getPid() && "null pid");

        // init VMM scatter
        VMMDLL_SCATTER_HANDLE hScatter = VMMDLL_Scatter_Initialize(
                this->pFPGA->getVmm(),
                this->getPid(),
                VMM_READ_FLAGS);
        if(!hScatter) return false;

        // push all reads into the scatter
        for (auto &read: readOps) {
            uint64_t addr = std::get<0>(read);
            uint8_t *buf = std::get<1>(read);
            uint32_t len = std::get<2>(read);

            // skip bad reads rather than fail out....
            if(!addr) continue;
            if(!buf) continue;
            if(!len) continue;

            if(!VMMDLL_Scatter_PrepareEx(
                    hScatter,
                    addr,
                    len,
                    buf,
                    nullptr))
            {
                VMMDLL_Scatter_CloseHandle(hScatter);
                return false;
            }
        }

        // execute read & clean up mem
        if (!VMMDLL_Scatter_ExecuteRead(hScatter)) {
            // read execution failed for some reason
            VMMDLL_Scatter_CloseHandle(hScatter);
            return false;
        }

        VMMDLL_Scatter_CloseHandle(hScatter);
        return true;
    }

    bool Process::Write(uint64_t addr, uint8_t *buffer, uint32_t size) {
        assert(this->pFPGA && "null fpga");
        assert(this->getPid() && "null pid");

        if(!addr) return false;
        if(!buffer) return false;
        if(!size) return false;

        return VMMDLL_MemWrite(
                this->pFPGA->getVmm(),
                this->getPid(),
                addr,
                buffer,
                size);
    }

    uint64_t Process::FindPattern(uint64_t start, uint64_t stop, uint8_t *pattern, uint8_t *mask) {
        assert(false && "not implemented");
    }

    uint64_t Process::GetModuleBase(const std::string &name) {
        assert(this->pFPGA && "null fpga");
        assert(this->getPid() && "null pid");

        return VMMDLL_ProcessGetModuleBaseU(
                this->pFPGA->getVmm(),
                this->getPid(),
                (char *) name.c_str());
    }

    bool Process::GetModuleInfo(const std::string &name, VMMDLL_MAP_MODULEENTRY &entry) {
        assert(this->pFPGA && "null fpga");
        assert(this->getPid() && "null pid");

        // reading will alloc a ptr we need to free
        PVMMDLL_MAP_MODULEENTRY pModuleMapEntry;
        bool ok = VMMDLL_Map_GetModuleFromNameU(
                this->pFPGA->getVmm(),
                this->getPid(),
                (char *) name.c_str(),
                &pModuleMapEntry,
                0);
        if (!ok) return false;

        // copy ptr data to input struct & give result
        if (!std::memcpy(&entry, pModuleMapEntry, sizeof(VMMDLL_MAP_MODULEENTRY))) {
            VMMDLL_MemFree(pModuleMapEntry);
            return false;
        }

        VMMDLL_MemFree(pModuleMapEntry);
        return true;
    }

    std::vector<VMMDLL_MAP_MODULEENTRY> Process::GetModules() {
        assert(this->pFPGA && "null fpga");
        assert(this->getPid() && "null pid");

        std::vector<VMMDLL_MAP_MODULEENTRY> results;

        PVMMDLL_MAP_MODULE pModuleMap = nullptr;
        if (!VMMDLL_Map_GetModuleU(this->pFPGA->getVmm(), this->getPid(), &pModuleMap, 0))
            return results;

        if (pModuleMap->dwVersion != VMMDLL_MAP_MODULE_VERSION) {
            VMMDLL_MemFree(pModuleMap);
            return results;
        }

        PVMMDLL_MAP_MODULEENTRY pModuleEntry;
        for (int i = 0; i < pModuleMap->cMap; i++) {
            pModuleEntry = pModuleMap->pMap + i;
            if (!pModuleEntry) continue;

            results.push_back(*pModuleEntry);
        }

        VMMDLL_MemFree(pModuleMap);
        return results;
    }

    uint64_t Process::FindCave(uint64_t start, uint64_t stop) {
        assert(false && "not implemented");
    }

    std::vector<VMMDLL_MAP_EATENTRY> Process::GetExports(const std::string &name) {
        assert(this->pFPGA && "null fpga");
        assert(this->getPid() && "null pid");

        std::vector<VMMDLL_MAP_EATENTRY> results;

        PVMMDLL_MAP_EAT pEAT = nullptr;
        bool ok = VMMDLL_Map_GetEATU(
                this->pFPGA->getVmm(),
                this->getPid(),
                (char*)name.c_str(),
                &pEAT);
        if(!ok) return results;

        if(pEAT->dwVersion != VMMDLL_MAP_EAT_VERSION) {
            VMMDLL_MemFree(pEAT);
            return results;
        }

        PVMMDLL_MAP_EATENTRY pEATEntry;
        for(int i = 0; i < pEAT->cMap; i++) {
            pEATEntry = pEAT->pMap + i;
            if(!pEATEntry) continue;

            results.push_back(*pEATEntry);
        }

        VMMDLL_MemFree(pEAT);
        return results;
    }

    std::vector<VMMDLL_MAP_IATENTRY> Process::GetImports(const std::string &name) {
        assert(this->pFPGA && "null fpga");
        assert(this->getPid() && "null pid");

        std::vector<VMMDLL_MAP_IATENTRY> results;

        PVMMDLL_MAP_IAT pIAT = nullptr;
        bool ok = VMMDLL_Map_GetIATU(
                this->pFPGA->getVmm(),
                this->getPid(),
                (char*)name.c_str(),
                &pIAT);
        if(!ok) return results;

        if(pIAT->dwVersion != VMMDLL_MAP_IAT_VERSION) {
            VMMDLL_MemFree(pIAT);
            return results;
        }

        PVMMDLL_MAP_IATENTRY pIATEntry;
        for(int i = 0; i < pIAT->cMap; i++) {
            pIATEntry = pIAT->pMap + i;
            if(!pIATEntry) continue;

            results.push_back(*pIATEntry);
        }

        VMMDLL_MemFree(pIAT);
        return results;
    }

    std::vector<VMMDLL_MAP_THREADENTRY> Process::GetThreads() {
        assert(this->pFPGA && "null fpga");
        assert(this->getPid() && "null pid");

        std::vector<VMMDLL_MAP_THREADENTRY> results;

        PVMMDLL_MAP_THREAD pThreads = nullptr;
        bool ok = VMMDLL_Map_GetThread(
                this->pFPGA->getVmm(),
                this->getPid(),
                &pThreads);
        if(!ok) return results;

        if(pThreads->dwVersion != VMMDLL_MAP_THREAD_VERSION) {
            VMMDLL_MemFree(pThreads);
            return results;
        }

        //TODO: for this pattern lets do RESIZE and copy data into the results vector
        // this will turn X allocations/resizes into only 1 regardless of cMap size
        PVMMDLL_MAP_THREADENTRY pThreadEntry;
        for(int i = 0; i < pThreads->cMap; i++) {
            pThreadEntry = pThreads->pMap + i;
            if(!pThreadEntry) continue;

            results.push_back(*pThreadEntry);
        }

        VMMDLL_MemFree(pThreads);
        return results;
    }

    uint64_t Process::GetExport(const std::string &moduleName, const std::string &exportName) {
        auto exports = this->GetExports(moduleName);
        for(auto &entry : exports) {
            // case insensitive compare desired name with actual export name
            std::string funcName(entry.uszFunction);
            if(std::equal(exportName.begin(), exportName.end(), funcName.begin(), [](char a, char b) {
                return std::tolower(a) == std::tolower(b);
            }))
            {
                return entry.vaFunction;
            }
        }
        return 0;
    }

    uint64_t Process::GetImport(const std::string &moduleName, const std::string &importName) {
        auto imports = this->GetImports(moduleName);
        for(auto &entry : imports) {
            // case insensitive compare desired name with actual export name
            std::string funcName(entry.uszFunction);
            if(std::equal(importName.begin(), importName.end(), funcName.begin(), [](char a, char b) {
                return std::tolower(a) == std::tolower(b);
            }))
            {
                return entry.vaFunction;
            }
        }
        return 0;
    }

} // memstream