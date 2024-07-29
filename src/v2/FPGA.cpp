#include <MemStream/v2/_macros.h>
#include <MemStream/v2/FPGA.h>
#include <MemStream/v2/Utils.h>

#include <print>

using namespace memstream::v2;

FPGA::FPGA() {
    _free_handle = true;
    init(); // initialize VMM
    readConfig(); // read config

    std::println("initialized successfully.");
    std::println("{}", _config.to_string().c_str());

// #ifdef MEMSTREAM_VERBOSE
//     std::println("config space:");
//     log::buffer(_config.configSpace, 0x1000);
//     std::println("");
// #endif

    disableMasterAbort(); // enable auto-clear status register
}
FPGA::~FPGA() {
    if(_free_handle) 
        VMMDLL_Close(_vmm);
}

FPGA::FPGA(VMM_HANDLE vmm) {   
    _free_handle = false;
    readConfig(); // read config
    disableMasterAbort(); // enable auto-clear status register
    _vmm = vmm;
}


// initialize vmm
void FPGA::init() {
    std::println("initializing fpga device...");

    // setup args
    LPCSTR args[] = {
        const_cast<LPCSTR>(""), 
        const_cast<LPCSTR>("-device"), 
        const_cast<LPCSTR>("fpga://algo=0"), 
        const_cast<LPCSTR>(""), 
        const_cast<LPCSTR>("")
    };
    int argc = 3;

#ifdef MEMSTREAM_VERBOSE
    args[argc++] = const_cast<LPCSTR>("-v");
    args[argc++] = const_cast<LPCSTR>("-printf");
#endif
     
    // initialize
    _vmm = VMMDLL_Initialize(argc, args);
    if (!_vmm) {
        LOG_ERR("failed to initialize vmmdll");
        throw std::runtime_error("VMMDLL_InitializeEx failed");
    }
}

// read config data from FPGA
void FPGA::readConfig() {
    // NOTE: only LC_OPT* can be acquired via VMMDLL_ConfigGet
    if (VMMDLL_ConfigGet(_vmm, LC_OPT_FPGA_FPGA_ID, &_config.deviceID) &&
        VMMDLL_ConfigGet(_vmm, LC_OPT_FPGA_VERSION_MAJOR, &_config.majorVer) &&
        VMMDLL_ConfigGet(_vmm, LC_OPT_FPGA_VERSION_MINOR, &_config.minorVer)) {
            
        // we don't really care if these succeed because nothing relies on it _internally_.
        // and we don't promise accuracy to the user for these.
        VMMDLL_ConfigGet(_vmm, LC_OPT_FPGA_ALGO_TINY, &_config.tiny);
        VMMDLL_ConfigGet(_vmm, LC_OPT_FPGA_FPGA_ID, &_config.fpgaID);
        VMMDLL_ConfigGet(_vmm, LC_OPT_FPGA_DELAY_READ, &_config.readDelay);
        VMMDLL_ConfigGet(_vmm, LC_OPT_FPGA_DELAY_WRITE, &_config.writeDelay);
        VMMDLL_ConfigGet(_vmm, LC_OPT_FPGA_MAX_SIZE_RX, &_config.maxSize);
        VMMDLL_ConfigGet(_vmm, LC_OPT_FPGA_RETRY_ON_ERROR, &_config.retry);

        // send some initial LC commands to pull more data about the FPGA device
        LC_CONFIG LcConfig = {};
        LcConfig.dwVersion = LC_CONFIG_VERSION;
        strcpy_s(LcConfig.szDevice, "existing");

        // fetch already existing leechcore handle.
        HANDLE hLC = LcCreate(&LcConfig);
        if(hLC) {
            PBYTE cfgSpace = nullptr;
            DWORD cfgSpaceSize = 0; // should always out 0x1000 but 🤷‍♂️S
            LcCommand(
                    hLC,
                    LC_CMD_FPGA_PCIECFGSPACE,
                    0,
                    nullptr,
                    &cfgSpace,
                    &cfgSpaceSize);

            if(cfgSpace && cfgSpaceSize > 0) {
                memcpy(_config.configSpace, cfgSpace, min(0x1000, cfgSpaceSize));
                LcMemFree(cfgSpace);
            } else {
                LOG_ERR("failed to query leechcore");
            }
            LcClose(hLC);
        } else {
            LOG_ERR("failed to create leechcore device");
        }
    } else {
        LOG_ERR("failed to get device information");
    }
}

void FPGA::disableMasterAbort() {
    if(!_config.CanAutoClearMasterAbort()) return; // not supported

    LC_CONFIG LcConfig = {};
    LcConfig.dwVersion = LC_CONFIG_VERSION;
    strcpy_s(LcConfig.szDevice, "existing");
    
    // fetch already existing leechcore handle.
    HANDLE hLC = LcCreate(&LcConfig);
    if (hLC) {
        // enable auto-clear of status register [master abort].
        BYTE dataIn[4] =  {0x10, 0x00, 0x10, 0x00};
        LcCommand(
                hLC,
                LC_CMD_FPGA_CFGREGPCIE_MARKWR | 0x002,
                4,
                dataIn,
                nullptr,
                nullptr);

        // close leechcore handle.
        LcClose(hLC);
    } else {
        LOG_ERR("failed to create leechcore device");
    }
}

// --- utilities

uint32_t FPGA::GetProcessId(const std::string& processName) {
    uint32_t pid = 0;

    if(VMMDLL_PidGetFromName(getVMM(), (LPSTR)processName.c_str(), (PDWORD)&pid)) {
        return pid;
    } else {
        LOG_ERR("failed to get process id");
    }

    return 0;
}

std::vector<uint32_t> FPGA::GetAllProcessIds(const std::string& processName) {
    PVMMDLL_PROCESS_INFORMATION process_info = NULL;
    DWORD total_processes = 0;
    std::vector<uint32_t> list;
    
    if (VMMDLL_ProcessGetInformationAll(getVMM(), &process_info, &total_processes)) {
        for (size_t i = 0; i < total_processes; i++) {
            if (strstr(process_info[i].szNameLong, processName.c_str())) {
                list.emplace_back(process_info[i].dwPID);
            }
        }

        VMMDLL_MemFree(process_info);
        return list;
    } else {
        LOG_ERR("failed to get all process information");
    }

    return {};
}

PVMMDLL_PROCESS_INFORMATION FPGA::GetProcessInfo(uint32_t pid) {
    PVMMDLL_PROCESS_INFORMATION result = nullptr;
    SIZE_T size = 0;

    // get size of proc info
    if(VMMDLL_ProcessGetInformation(_vmm, pid, nullptr, &size)) {
        result = (PVMMDLL_PROCESS_INFORMATION)malloc(size);
        if(result) {
            memset(result, 0, size); // zero out
            result->magic = VMMDLL_PROCESS_INFORMATION_MAGIC;
            result->wVersion = VMMDLL_PROCESS_INFORMATION_VERSION;

            // populate struct
            if(VMMDLL_ProcessGetInformation(_vmm, pid, result, &size)) {
                return result;
            } else {
                LOG_ERR("failed to get process information");
            }
            
            free(result);
        } else {
            LOG_ERR("failed to allocate memory for process information");
        }
    } else {
        LOG_ERR("failed to get process information size");
    }

    return nullptr;
}

void FPGA::Read(uint32_t pid, uint64_t address, uint8_t* buffer, size_t size) {
    if(size && address && buffer) {
        DWORD bytesRead = 0;
        if(VMMDLL_MemReadEx(getVMM(), pid, address, buffer, size, &bytesRead, READ_FLAGS)) {
            if (bytesRead == size) {
                return;
            } else {
                LOG_ERR( "failed to read memory, missing bytes");
            }
        } else {
            LOG_ERR("failed to read memory, false returned");
        }
    } else {
        LOG_ERR("read args invalid");
    }

    return;
}

void FPGA::Write(uint32_t pid, uint64_t address, uint8_t* buffer, size_t size) {
    if(size && address && buffer) {
        if(VMMDLL_MemWrite(getVMM(), pid, address, buffer, size)) {
            return;
        } else {
            LOG_ERR("failed to write memory, false returned");
        }
    } else {
        LOG_ERR("write args invalid");
    }
   
     return;
}

PVMMDLL_MAP_MODULEENTRY FPGA::GetModuleInfo(uint32_t pid, const std::string& moduleName) {
    PVMMDLL_MAP_MODULEENTRY pModuleMapEntry = nullptr;

    if(VMMDLL_Map_GetModuleFromNameU(getVMM(), pid, moduleName.c_str(), &pModuleMapEntry, 0)) {        
        // LEAKS !
        return pModuleMapEntry;
    } else {
        // logs the base PID for this module even if we're trying to find a kernel module
        LOG_ERR("failed to get module information for {} in {}", moduleName.c_str(), (pid &~ VMMDLL_PID_PROCESS_WITH_KERNELMEMORY));
    }

    return nullptr;
}

PVMMDLL_MAP_MODULE FPGA::GetModules(uint32_t pid) {
    PVMMDLL_MAP_MODULE modules = nullptr;

    if(VMMDLL_Map_GetModuleU(getVMM(), pid, &modules, 0)) {
        if(modules->dwVersion == VMMDLL_MAP_MODULE_VERSION) {
            if(modules->cMap > 0) {
                // LEAKS !
                return modules;
            } else {
                LOG_ERR("no modules found");
            }
        } else {
            LOG_ERR("module version mismatch");
        }

        VMMDLL_MemFree(modules);
    } else {
        LOG_ERR("failed to get modules");
    }

    return nullptr;
}

uint64_t FPGA::GetProcAddress(uint32_t pid, const std::string& moduleName, const std::string& procName) {
    if(!moduleName.empty() && !procName.empty()) {
        uint64_t addr = VMMDLL_ProcessGetProcAddressU(getVMM(), pid, moduleName.c_str(), procName.c_str());
        if(addr) {
            return addr;
        } else {
            LOG_ERR("failed to get process address");
        }
    } else {
        LOG_ERR("module name or proc name cannot be empty");
    }

    return 0;
}

// ------------- WINDOWS REGISTRY STUFF ⬇️⬇️⬇️

uint8_t* FPGA::QueryWindowsRegistry(const std::string& path, uint32_t valueType) {
    DWORD size = 0;
    if(VMMDLL_WinReg_QueryValueExU(_vmm, path.c_str(), (LPDWORD)&valueType, nullptr, &size)) {
        if(size > 0) {
            uint8_t* buffer = (uint8_t*)malloc(size);
            if(buffer) {
                if(VMMDLL_WinReg_QueryValueExU(_vmm, path.c_str(), (LPDWORD)&valueType, buffer, &size)) {
                    return buffer;
                } else {
                    LOG_ERR("failed to query registry value");
                }
            } else {
                LOG_ERR("failed to allocate memory for registry value");
            }
        } else {
            return nullptr;
        }
    } else {
        LOG_ERR("failed to query registry value size");

    }

    return nullptr;
}