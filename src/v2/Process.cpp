#include <MemStream/v2/_macros.h>
#include <MemStream/v2/Process.h>

#include <stdexcept>
#include <sstream>
using namespace memstream::v2;

Process::Process(FPGA* fpga, uint32_t pid, bool kernel) {
    _fpga = fpga;
    _pid = pid;
    _kernel = kernel;

    _info = _fpga->GetProcessInfo(_pid);
    _scatter = VMMDLL_Scatter_Initialize(_fpga->getVMM(), readPID(), READ_FLAGS);
}
Process::~Process() {
    free(_info);
    VMMDLL_Scatter_CloseHandle(_scatter);
}

// ------------- SCATTER READING ⬇️⬇️⬇️

void Process::PrepareRead(uint64_t addr, uint8_t* buffer, size_t size) {
    memset(buffer, 0, size); // ensure buffer is zero !
    if(VMMDLL_Scatter_PrepareEx(_scatter, addr, size, buffer, nullptr)) {
        return;
    } else {
        LOG_ERR("Failed to prepare scatter");
    }

    return;
}

void Process::Read() {
    if(!VMMDLL_Scatter_ExecuteRead(_scatter)) {
        LOG_ERR("Failed to execute read");
    }
    if(!VMMDLL_Scatter_Clear(_scatter, readPID(), READ_FLAGS)) {
        LOG_ERR("Failed to clear scatter");
    }

    return;  
}

void Process::ReadOnce(uint64_t addr, uint8_t* buffer, size_t size) {
    DWORD readBytes = 0;
    if(VMMDLL_MemReadEx(_fpga->getVMM(), readPID(), addr, buffer, size, &readBytes, READ_FLAGS)) {
        if(readBytes == size) {
            return;
        } else {
            LOG_ERR("failed to read all bytes. only read {:#x} of {:#x}", readBytes, size);
        }
    } else {
        LOG_ERR("failed to read memory");
    }
    return;
}

// ------------- SCATTER READING ⬆️⬆️⬆️
// ------------- SCATTER WRITING ⬇️⬇️⬇️

void Process::PrepareWrite(uint64_t addr, uint8_t* buffer, size_t size) {
    if(!VMMDLL_Scatter_PrepareWriteEx(_scatter, addr, buffer, size)) {
        LOG_ERR("failed to prepare scatter write");
    }

    return;
}

void Process::Write() {
    if(!VMMDLL_Scatter_Execute(_scatter)) {
        LOG_ERR("Failed to execute write");
    }
    if(!VMMDLL_Scatter_Clear(_scatter, readPID(), READ_FLAGS)) {
        LOG_ERR("Failed to clear scatter");
    }

    return;
}

void Process::WriteOnce(uint64_t addr, uint8_t* buffer, size_t size) {
    if(!VMMDLL_MemWrite(_fpga->getVMM(), readPID(), addr, buffer, size)) {
        LOG_ERR("failed to read memory");
    }
    return;
}

// ------------- SCATTER WRITING ⬆️⬆️⬆️
// ------------- WINDOWS THINGYS ⬇️⬇️⬇️ 

uint64_t Process::GetExport(const std::string& module, const std::string& entry) {
    return _fpga->GetProcAddress(readPID(), module, entry);
}

uint64_t Process::GetImport(const std::string& module, const std::string& entry) {
    // TODO: implement !
    return 0;
}

uint64_t Process::GetModuleBase(const std::string& module) {
    auto info = _fpga->GetModuleInfo(readPID(), module);
    if(!info) return 0; //  failed to get module base :( 
    uint64_t base = info->vaBase;
    VMMDLL_MemFree(info);
    return base;
}

bool cmp_bytes(const uint8_t* data, uint8_t* pattern, const char* mask) {
    for (; *mask; ++pattern, ++mask, ++data) {
        if (*mask == 'x' && *pattern != *data) {
            return false;
        }
    }
    return (*mask) == '\0';
}
uint8_t* find_pattern(uint8_t* data, size_t dataSize, uint8_t* pattern, const char* mask) {
    size_t patternLength = strlen(mask);
    for (size_t i = 0; i <= dataSize - patternLength; ++i) {
        if (cmp_bytes(data + i, pattern, mask)) {
            return data + i;
        }
    }
    return nullptr;
}

uint64_t Process::FindPattern(const std::string& module, uint8_t* pattern, const char* mask) {
    // -- get base address of module (for section calculation)
    uint64_t base = this->GetModuleBase(module);

    // -- get all section information for the module
    DWORD cSections = 0;
    PIMAGE_SECTION_HEADER pSectionHeaders = nullptr;

    if(!VMMDLL_ProcessGetSectionsU(this->_fpga->getVMM(), readPID(), module.c_str(), NULL, 0, &cSections)) {
        LOG_ERR("failed to get section count for '{}'", module.c_str());
        return 0;
    }
    pSectionHeaders = new IMAGE_SECTION_HEADER[cSections](); //(PIMAGE_SECTION_HEADER)LocalAlloc(LMEM_ZEROINIT, cSections * sizeof(IMAGE_SECTION_HEADER));
    if(!pSectionHeaders) {
        LOG_ERR("localalloc for section headers failed for count {}", cSections);
        return 0;
    }
    if(!VMMDLL_ProcessGetSectionsU(this->_fpga->getVMM(), readPID(), module.c_str(), pSectionHeaders, cSections, &cSections)) {
        LOG_ERR("failed to get sections for '{}'", module.c_str());
        return 0;
    }

    // -- search each section for the pattern
    uint64_t result = 0;

    uint8_t* buffer = nullptr;
    uint32_t buffer_size = 0;
    for(int i = 0; i < cSections; i++) {
        const auto& section = pSectionHeaders[i];
        if((section.Characteristics & IMAGE_SCN_MEM_EXECUTE) == 0) continue; // only scan executable sections

        // get section start address & size
        uint64_t addr = base + section.VirtualAddress;
        uint32_t size = section.Misc.VirtualSize;
        std::printf("Looking at %llX %i\n", addr, size);

        // read section into a buffer in our process
        if(buffer_size < size) {
            delete[] buffer;
            buffer = new uint8_t[size]();
        }
        this->ReadOnce(addr, buffer, size);

        // look for the pattern in the section
        //uint8_t* found_ptr = find_pattern(buffer, size, pattern.c_str(), mask.c_str());
        uint8_t* found_ptr = find_pattern(buffer, size, pattern, mask);
        if(found_ptr) {
            // figure out offset the pattern was found at & return the VirtualAddress from the process
            uint32_t offset = (found_ptr - buffer);
            result = addr + offset;
            break;
        }
    }
    delete[] buffer;
    buffer = nullptr;

    delete[] pSectionHeaders;
    pSectionHeaders = nullptr;

    return result;
}
