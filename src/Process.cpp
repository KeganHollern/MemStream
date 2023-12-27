//
// Created by Kegan Hollern on 12/23/23.
//
#include <cstdint>
#include <cassert>
#include <sstream>
#include <string>
#include <cstring>
#include <tuple>
#include <vector>
#include <fstream>
#include <stdexcept>

#include <vmmdll.h>

#include "MemStream/Utils.h"
#include "MemStream/FPGA.h"
#include "MemStream/Process.h"

namespace memstream {
    Process::Process(uint32_t pid) : Process(GetDefaultFPGA(), pid) {}

    Process::Process(FPGA *pFPGA, uint32_t pid) : info() {
        if (!pFPGA)
            throw std::invalid_argument("invalid fpga provided");

        if (!pFPGA->GetProcessInfo(pid, this->info))
            throw std::runtime_error("failed to get process info");

        this->pFPGA = pFPGA;
    }

    Process::Process(const std::string &name) : Process(GetDefaultFPGA(), name) {}

    Process::Process(FPGA *pFPGA, const std::string &name) : info() {
        if (!pFPGA)
            throw std::invalid_argument("invalid fpga provided");

        uint32_t foundPid = 0;
        if (!VMMDLL_PidGetFromName(pFPGA->getVmm(), (char *) name.c_str(), &foundPid))
            throw std::runtime_error("failed to find process with name");

        if (!pFPGA->GetProcessInfo(foundPid, this->info))
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

    void Process::StageRead(uint64_t addr, uint8_t *buffer, uint32_t size) {
        this->stagedReads.emplace_back(addr, buffer, size);
    }

    bool Process::ExecuteStagedReads() {
        bool result = this->ReadMany(this->stagedReads);
        this->stagedReads.clear();
        return result;
    }

    bool Process::Read(uint64_t addr, uint8_t *buffer, uint32_t size) {
        assert(this->pFPGA && "null fpga");
        assert(this->getPid() && "null pid");

        if (!addr) return false;
        if (!buffer) return false;
        if (!size) return false;

        return VMMDLL_MemReadEx(
                this->pFPGA->getVmm(),
                this->getPid(),
                addr,
                buffer,
                size,
                nullptr,
                VMM_READ_FLAGS);
    }

    bool Process::ReadMany(std::vector<std::tuple<uint64_t, uint8_t *, uint32_t>> &readOps) {
        assert(this->pFPGA && "null fpga");
        assert(this->getPid() && "null pid");

        // init VMM scatter
        VMMDLL_SCATTER_HANDLE hScatter = VMMDLL_Scatter_Initialize(
                this->pFPGA->getVmm(),
                this->getPid(),
                VMM_READ_FLAGS);
        if (!hScatter) return false;

        // push all reads into the scatter
        for (auto &read: readOps) {
            uint64_t addr = std::get<0>(read);
            uint8_t *buf = std::get<1>(read);
            uint32_t len = std::get<2>(read);

            // skip bad reads rather than fail out....
            if (!addr) continue;
            if (!buf) continue;
            if (!len) continue;

            if (!VMMDLL_Scatter_PrepareEx(
                    hScatter,
                    addr,
                    len,
                    buf,
                    nullptr)) {
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

        if (!addr) return false;
        if (!buffer) return false;
        if (!size) return false;

        return VMMDLL_MemWrite(
                this->pFPGA->getVmm(),
                this->getPid(),
                addr,
                buffer,
                size);
    }

    void Process::StageWrite(uint64_t addr, uint8_t *buffer, uint32_t size) {
        this->stagedWrites.emplace_back(addr, buffer, size);
    }

    bool Process::ExecuteStagedWrites() {
        bool result = this->WriteMany(this->stagedWrites);
        this->stagedWrites.clear();
        return result;
    }

    bool Process::WriteMany(std::vector<std::tuple<uint64_t, uint8_t *, uint32_t>> &writeOps) {
        assert(this->pFPGA && "null fpga");
        assert(this->getPid() && "null pid");

        // init VMM scatter
        VMMDLL_SCATTER_HANDLE hScatter = VMMDLL_Scatter_Initialize(
                this->pFPGA->getVmm(),
                this->getPid(),
                VMM_READ_FLAGS);
        if (!hScatter) return false;

        // push all writes into the scatter
        for (auto &write: writeOps) {
            uint64_t addr = std::get<0>(write);
            uint8_t *buf = std::get<1>(write);
            uint32_t len = std::get<2>(write);

            // skip bad writes rather than fail out....
            if (!addr) continue;
            if (!buf) continue;
            if (!len) continue;

            if (!VMMDLL_Scatter_PrepareWrite(
                    hScatter,
                    addr,
                    buf,
                    len)) {
                VMMDLL_Scatter_CloseHandle(hScatter);
                return false;
            }
        }

        // execute write & clean up mem
        if (!VMMDLL_Scatter_Execute(hScatter)) {
            // write execution failed for some reason
            VMMDLL_Scatter_CloseHandle(hScatter);
            return false;
        }

        VMMDLL_Scatter_CloseHandle(hScatter);
        return true;
    }

    // ex: IDA pattern: 00 0A FF FF ?? ?? ?? ?? C3

    std::vector<std::tuple<uint8_t, bool>> Process::parsePattern(const std::string &pattern) {
        std::vector<std::tuple<uint8_t, bool>> result;

        std::istringstream iss(pattern);
        std::string hexStr;

        while (iss >> hexStr) {
            if (hexStr.empty()) continue;
            if (hexStr == "?" || hexStr == "??") {
                result.emplace_back(0, false);
            } else {
                auto byte = static_cast<uint8_t>(std::stoul(hexStr, nullptr, 16));
                result.emplace_back(byte, true);
            }
        }

        return result;
    }

    uint64_t Process::FindPattern(uint64_t start, uint64_t stop, const std::string &pattern) {
        auto search = this->parsePattern(pattern);

        uint32_t len = stop - start;

        // will this blow the stack?
        uint8_t buffer[len];
        if (!this->Read(start, buffer, len))
            return 0;


        for (int i = 0; i < len - search.size(); i++) {
            bool found = true;
            for (int j = 0; j < search.size(); j++) {
                assert(i + j < len && "invalid pattern search range");

                auto &entry = search[j];

                if (!std::get<1>(entry)) continue; // wildcard

                if (std::get<0>(entry) != buffer[i + j]) {
                    found = false;
                    break; // nonmatching required byte
                }
            }
            if (found) {
                return (start + i);
            }
        }

        return 0;
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

    bool Process::Dump(const std::string& path) {
        assert(this->pFPGA && "null fpga");
        assert(this->getPid() && "null pid");

        assert(false && "todo impl");
    }

    // TODO: move some of these windows specific fncs into a WinProcess child class?

    uint64_t Process::Cave(const std::string& moduleName, uint32_t size) {
        assert(this->pFPGA && "null fpga");
        assert(this->getPid() && "null pid");

        const uint32_t pad = 0x10; // cave padding

        // CANNOT find caves larger than an entire page!
        // TODO: define page size (0x1000) in some header
        //  and with this define PAGE_SIZE-0x1 alongside it for bitwise calcs
        //  utils ?
        if(size+(pad*2) >= 0x1000) return 0;

        DWORD dwSectionCount = 0;
        bool ok = VMMDLL_ProcessGetSectionsU(
                this->pFPGA->getVmm(),
                this->getPid(),
                (char*)moduleName.c_str(),
                nullptr,
                0,
                &dwSectionCount);
        if(!ok) return 0;

        // no sections for module?
        if(dwSectionCount == 0) return 0;

        // will this blow the stack? (probably not..)
        IMAGE_SECTION_HEADER sections[dwSectionCount];

        ok = VMMDLL_ProcessGetSectionsU(
                this->pFPGA->getVmm(),
                this->getPid(),
                (char*)moduleName.c_str(),
                sections,
                dwSectionCount,
                &dwSectionCount);
        if(!ok) return 0;

        // max possible cave is 0xfff in size
        // well its actually smaller bcz of our pad
        // but if we set pad to 0 then this actually is...
        uint8_t cave_buffer[0xfff];

        for(IMAGE_SECTION_HEADER& section : sections) {
            // only sections with enough free space at the tail of the page
            // (pad our actual fnc size by PAD on each side)
            uint32_t free_bytes = 0x1000 - (section.Misc.VirtualSize & 0xfff);
            if(free_bytes < (size+(pad*2)))
                continue;

            // RWX sections only (wouldn't RX work too?)
            if(!(section.Characteristics & (IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_WRITE | IMAGE_SCN_MEM_READ)))
                continue;

            uint64_t section_start = this->GetModuleBase(moduleName) + section.VirtualAddress;
            uint64_t section_end = section_start + section.Misc.VirtualSize;
            uint64_t cave_addr = section_end + pad;

            // TODO: read entire remainder of cave page...
            //      uint64_t read_size = cave_addr - page::next(cave_addr) - pad;
            //  and find anywhere with "size" repeating 0x0 values in that read
            //  return address of start of that repeating 0x0 chain

            uint64_t read_stop = page::next(cave_addr) - pad;
            uint64_t read_size = read_stop - cave_addr + 0x1; // the 1 here should be correct
            assert(read_size <= sizeof(cave_buffer) && "bad read size calculated");
            assert(read_size <= size && "read size smaller than required");

            // read cave
            std::memset(cave_buffer, 0, sizeof(cave_buffer));
            if(!this->Read(cave_addr, cave_buffer, read_size))
                continue;

            // scan the entire cave for a large enough pattern of 0s
            for(uint32_t i = 0; i < read_size-size;) {
                bool found = true;

                for(uint32_t j = 0; j < size;j++) {
                    auto& byte = cave_buffer[i+j];
                    if(byte != 0x0) {
                        found = false;
                        // non-zero
                        // next itr start from next byte
                        i += j+1;
                        break;
                    }
                }

                // if we have a large enough cave lets return it
                if(found) {
                    // need to shift start of cave to the offset we found a valid cave @
                    // often this will be [0]
                    cave_addr += i;
                    return cave_addr;
                }
            } // foreach BYTE IN CAVE BUFFER
        } // foreach SECTION

        // no valid cave in sections
        return 0;
    }

    std::vector<VMMDLL_MAP_EATENTRY> Process::GetExports(const std::string &name) {
        assert(this->pFPGA && "null fpga");
        assert(this->getPid() && "null pid");

        std::vector<VMMDLL_MAP_EATENTRY> results;

        PVMMDLL_MAP_EAT pEAT = nullptr;
        bool ok = VMMDLL_Map_GetEATU(
                this->pFPGA->getVmm(),
                this->getPid(),
                (char *) name.c_str(),
                &pEAT);
        if (!ok) return results;

        if (pEAT->dwVersion != VMMDLL_MAP_EAT_VERSION) {
            VMMDLL_MemFree(pEAT);
            return results;
        }

        PVMMDLL_MAP_EATENTRY pEATEntry;
        for (int i = 0; i < pEAT->cMap; i++) {
            pEATEntry = pEAT->pMap + i;
            if (!pEATEntry) continue;

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
                (char *) name.c_str(),
                &pIAT);
        if (!ok) return results;

        if (pIAT->dwVersion != VMMDLL_MAP_IAT_VERSION) {
            VMMDLL_MemFree(pIAT);
            return results;
        }

        PVMMDLL_MAP_IATENTRY pIATEntry;
        for (int i = 0; i < pIAT->cMap; i++) {
            pIATEntry = pIAT->pMap + i;
            if (!pIATEntry) continue;

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
        if (!ok) return results;

        if (pThreads->dwVersion != VMMDLL_MAP_THREAD_VERSION) {
            VMMDLL_MemFree(pThreads);
            return results;
        }

        //TODO: for this pattern lets do RESIZE and copy data into the results vector
        // this will turn X allocations/resizes into only 1 regardless of cMap size
        PVMMDLL_MAP_THREADENTRY pThreadEntry;
        for (int i = 0; i < pThreads->cMap; i++) {
            pThreadEntry = pThreads->pMap + i;
            if (!pThreadEntry) continue;

            results.push_back(*pThreadEntry);
        }

        VMMDLL_MemFree(pThreads);
        return results;
    }

    uint64_t Process::GetExport(const std::string &moduleName, const std::string &exportName) {
        auto exports = this->GetExports(moduleName);
        for (auto &entry: exports) {
            // case insensitive compare desired name with actual export name
            std::string funcName(entry.uszFunction);
            if (std::equal(exportName.begin(), exportName.end(), funcName.begin(), [](char a, char b) {
                return std::tolower(a) == std::tolower(b);
            })) {
                return entry.vaFunction;
            }
        }
        return 0;
    }

    uint64_t Process::GetImport(const std::string &moduleName, const std::string &importName) {
        auto imports = this->GetImports(moduleName);
        for (auto &entry: imports) {
            // case insensitive compare desired name with actual export name
            std::string funcName(entry.uszFunction);
            if (std::equal(importName.begin(), importName.end(), funcName.begin(), [](char a, char b) {
                return std::tolower(a) == std::tolower(b);
            })) {
                return entry.vaFunction;
            }
        }
        return 0;
    }


} // memstream