#include <sstream>
#include <string>
#include <cstring>
#include <tuple>
#include <vector>
#include <stdexcept>
#include <list>
#include <iostream>

#include <vmmdll.h>

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
        this->pid = pid;

        this->scatter = VMMDLL_Scatter_Initialize(
                this->pFPGA->getVmm(),
                this->getPid(),
                this->readFlags);

        if (!this->scatter)
            throw std::runtime_error("failed to initialize scatter for process");
    }

    Process::Process(const std::string &name) : Process(GetDefaultFPGA(), name) {}

    Process::Process(FPGA *pFPGA, const std::string &name) : info() {
        if (!pFPGA)
            throw std::invalid_argument("invalid fpga provided");

        uint32_t foundPid = 0;
        if (!VMMDLL_PidGetFromName(pFPGA->getVmm(), (char *) name.c_str(), (PDWORD)&foundPid)) {
            auto procs = pFPGA->GetAllProcessesByName(name);
            if(procs.empty()) {
                throw std::runtime_error("failed to find process with name (" + name + ")");
            } else {
                foundPid = procs[0];
            }
        }

        if (!pFPGA->GetProcessInfo(foundPid, this->info))
            throw std::runtime_error("failed to get process info");

        this->pid = foundPid;
        this->pFPGA = pFPGA;

        this->scatter = VMMDLL_Scatter_Initialize(
                this->pFPGA->getVmm(),
                this->getPid(),
                this->readFlags);
        if (!this->scatter)
            throw std::runtime_error("failed to initialize scatter for process");
    }

    Process::~Process() {
        if(this->scatter) {
            VMMDLL_Scatter_CloseHandle(this->scatter);
            this->scatter = nullptr;
        }
    }

    bool Process::isIs64Bit() const {
        return this->info.tpMemoryModel == VMMDLL_MEMORYMODEL_X64;
    }

    uint32_t Process::getSessionId() const {
        return this->info.win.dwSessionId;
    }

    uint32_t Process::getPid() const {
        return this->pid;
    }
    
    void Process::setVerbose(bool verbose) {
        this->verbose = verbose;
    }

    void Process::StageRead(uint64_t addr, uint8_t *buffer, uint32_t size) {
        if(!addr || !buffer || !size) return;

        this->stagedReads.push_front(std::make_shared<ScatterOp>(addr, buffer, size));
    }

    std::list<uint64_t> Process::ExecuteStagedReads() {

        if(this->stagedReads.empty()) return {};
        bool result = true;
        int attempts = 0;
        while(!this->stagedReads.empty()) {
            attempts++;
            
            // scatter read all data
            this->ReadMany(this->stagedReads);
            
            auto totalReads = this->stagedReads.size();
            // remove all successful reads and we'll retry if
            // any failed
            // also remove any invalid reads because they wouldn't have been read
            this->stagedReads.remove_if([](const std::shared_ptr<ScatterOp>& op){ 
                return !op->Valid() || op->size == op->cbRead; });

            if(!this->stagedReads.empty()) {
                if(this->verbose) {
                    std::cout << "failed " << 
                        this->stagedReads.size() << 
                        " scatter reads of " << 
                        totalReads <<
                        " on attempt " <<
                        attempts << std::endl;
                }

                if(attempts == 2) {
                    // failed 2 scatter reads -> try to normal read all remaining data
                    std::list<uint64_t> failList;
                    for(const auto& item : this->stagedReads) {
                        // one last non-scatter attempt
                        if(!this->Read(item->address, item->buffer, item->size)) {
                            // null the failed read buffer
                            memset(item->buffer, 0, item->size); 
                            failList.push_back(item->address);
                        }
                    }
                    this->stagedReads.clear();
                    return failList;
                }
            }
        }

        return {};
    }

    bool Process::Read(uint64_t addr, uint8_t *buffer, uint32_t size) {
        if (!addr) return false;
        if (!buffer) return false;
        if (!size) return false;

        DWORD read = 0;
        return VMMDLL_MemReadEx(
                this->pFPGA->getVmm(),
                this->getPid(),
                addr,
                buffer,
                size,
                &read,
                this->readFlags) && (read == size);
    }

    bool Process::ReadMany(std::list<std::shared_ptr<ScatterOp>> &operations) {
        // initialize a scatter
        if(!this->scatter) {
            auto new_scatter = VMMDLL_Scatter_Initialize(
                    this->pFPGA->getVmm(),
                    this->getPid(),
                    this->readFlags);

            if (!new_scatter) return false;
            this->scatter = new_scatter;
        }

        VMMDLL_Scatter_Clear(this->scatter, this->getPid(), this->readFlags);

        // TODO: optimized buckets for reading w/ multiple
        //  DMA scatters.

        // push all reads into the scatter
        bool something_to_read = false;

        std::list<std::shared_ptr<ScatterOp>>::iterator it;
        for (it = operations.begin(); it != operations.end(); ++it){
            if(!(*it)->Valid()) continue;
            if(!(*it)->PrepareRead(this->scatter)) {
                return false;
            }
            something_to_read = true;
        }

        // nothing to read
        // return success
        if(!something_to_read)
            return true;

        // execute read
        if(!VMMDLL_Scatter_ExecuteRead(this->scatter))
            return false;
    
        return true;   
    }

    bool Process::Write(uint64_t addr, uint8_t *buffer, uint32_t size) {
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
        if(!addr || !buffer || !size) return;

        this->stagedWrites.push_front(std::make_shared<ScatterOp>(addr, buffer, size));
    }

    bool Process::ExecuteStagedWrites() {
        bool result = this->WriteMany(this->stagedWrites);
        this->stagedWrites.clear();
        return result;
    }

    bool Process::WriteMany(std::list<std::shared_ptr<ScatterOp>>& operations) {
        // reinit VMM scatter
        if(!this->scatter) {
            this->scatter = VMMDLL_Scatter_Initialize(
                    this->pFPGA->getVmm(),
                    this->getPid(),
                    this->readFlags);
            if(!this->scatter) return false;
        }

        VMMDLL_Scatter_Clear(this->scatter, this->getPid(), this->readFlags);

        bool something_to_write = false;
        // push all writes into the scatter
        
        std::list<std::shared_ptr<ScatterOp>>::iterator it;
        for (it = operations.begin(); it != operations.end(); ++it){
            if(!(*it)->Valid()) continue;
            if(!(*it)->PrepareWrite(this->scatter)) {
                return false;
            }
            something_to_write = true;
        }

        if(!something_to_write) 
            return true;

        // execute write & clean up mem
        if (!VMMDLL_Scatter_Execute(this->scatter))
            return false;

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
        auto search = memstream::Process::parsePattern(pattern);

        uint32_t len = stop - start;

        auto buffer = new uint8_t[len]();
        if (!this->Read(start, buffer, len))
            return 0;


        for (int i = 0; i < len - search.size(); i++) {
            bool found = true;
            for (int j = 0; j < search.size(); j++) {
                auto &entry = search[j];

                if (!std::get<1>(entry)) continue; // wildcard

                if (std::get<0>(entry) != buffer[i + j]) {
                    found = false;
                    break; // nonmatching required byte
                }
            }
            if (found) {
                delete[] buffer;
                return (start + i);
            }
        }

        delete[] buffer;
        return 0;
    }

    uint64_t Process::GetModuleBase(const std::string &name) {
        return VMMDLL_ProcessGetModuleBaseU(
                this->pFPGA->getVmm(),
                this->getPid(),
                (char *) name.c_str());
    }

    bool Process::GetModuleInfo(const std::string &name, VMMDLL_MAP_MODULEENTRY &entry) {
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
            const auto& entry = pModuleMap->pMap[i];
            results.emplace_back(entry);
        }

        VMMDLL_MemFree(pModuleMap);
        return results;
    }

    bool Process::Dump(const std::string& path) {
        return false;
    }

    // TODO: move some of these windows specific fncs into a WinProcess child class?

    uint64_t Process::Cave(const std::string& moduleName, uint32_t size) {
        const uint32_t pad = 0x10; // cave padding

        // CANNOT find caves larger than an entire page!
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
        auto sections = (IMAGE_SECTION_HEADER*)alloca(sizeof(IMAGE_SECTION_HEADER)*dwSectionCount);

        ok = VMMDLL_ProcessGetSectionsU(
                this->pFPGA->getVmm(),
                this->getPid(),
                (char*)moduleName.c_str(),
                sections,
                dwSectionCount,
                &dwSectionCount);
        if(!ok) return 0;

        auto cave_buffer = (uint8_t*)alloca(size);

        for(DWORD i = 0; i < dwSectionCount;i++) {
            IMAGE_SECTION_HEADER& section = sections[i];
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

            // read cave
            std::memset(cave_buffer, 0, size);
            if(!this->Read(cave_addr, cave_buffer, size))
                continue;

            // check if read data contains any non-zero byte
            bool cave_ok = true;
            for(uint32_t j = 0; j < size; j++) {
                if(cave_buffer[j] != 0x0)
                {
                    cave_ok = false;
                    break;
                }
            }

            // return cave address if buffer contained all 0s
            if(cave_ok) return cave_addr;
        }

        // no valid cave in sections
        return 0;
    }

    std::vector<VMMDLL_MAP_EATENTRY> Process::GetExports(const std::string &name) {
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

        for (int i = 0; i < pEAT->cMap; i++) {
            const auto& entry = pEAT->pMap[i];
            results.emplace_back(entry);
        }

        VMMDLL_MemFree(pEAT);
        return results;
    }

    std::vector<VMMDLL_MAP_IATENTRY> Process::GetImports(const std::string &name) {
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

        for (int i = 0; i < pIAT->cMap; i++) {
            const auto& entry = pIAT->pMap[i];
            results.emplace_back(entry);
        }

        VMMDLL_MemFree(pIAT);
        return results;
    }

    std::vector<VMMDLL_MAP_THREADENTRY> Process::GetThreads() {
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
        for (int i = 0; i < pThreads->cMap; i++) {
            const auto& entry = pThreads->pMap[i];
            
            results.emplace_back(entry);
        }

        VMMDLL_MemFree(pThreads);
        return results;
    }

    uint64_t Process::GetExport(const std::string &moduleName, const std::string &exportName) {
        return VMMDLL_ProcessGetProcAddressU(
                this->pFPGA->getVmm(),
                this->getPid(),
                (char*)moduleName.c_str(),
                (char*)exportName.c_str());
    }

    uint64_t Process::GetImport(const std::string &moduleName, const std::string &importName) {
        auto imports = this->GetImports(moduleName);
        for (auto &entry: imports) {
            // case insensitive compare desired name with actual export name
            std::string funcName(entry.uszFunction);
            if (importName.size() == funcName.size() && std::equal(importName.begin(), importName.end(), funcName.begin(), [](char a, char b) {
                return std::tolower(a) == std::tolower(b);
            })) {
                return entry.vaFunction;
            }
        }
        return 0;
    }

    const char *Process::getName() const {
        return this->info.szNameLong;
    }


    ScatterOp::ScatterOp(uint64_t address, uint32_t size) {
        this->allocated = true;
        this->buffer = new uint8_t[size]();
        this->address = address;
        this->size = size;
    }
    ScatterOp::ScatterOp(uint64_t address, uint8_t* buffer, uint32_t size) {
        this->allocated = false;
        this->buffer = buffer;
        this->address = address;
        this->size = size;
    }
    ScatterOp::~ScatterOp() {
        if(this->allocated)
            delete[] this->buffer;
    }
} // memstream