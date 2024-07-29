#pragma once

#include "FPGA.h"

namespace memstream::v2
{
    class Process {
    private:
        bool _kernel;
        uint32_t _pid;
        FPGA* _fpga;

        PVMMDLL_PROCESS_INFORMATION _info;
        VMMDLL_SCATTER_HANDLE _scatter;

        inline int readPID() {
            return _kernel ? _pid | VMMDLL_PID_PROCESS_WITH_KERNELMEMORY : _pid;
        }
    public:
        Process(FPGA* fpga, uint32_t pid, bool kernel = false);
        ~Process();

        inline int GetPID() {
            return this->readPID();
        }

        // reading (scatter based)
        void PrepareRead(uint64_t addr, uint8_t* buffer, size_t size);
        template <typename T>
        void PrepareRead(uint64_t addr, T& buffer) {
            this->PrepareRead(addr, (uint8_t*)&buffer, sizeof(T));
        }
        
        // execute read (scatter)
        void Read();

        // reading (single offset)
        void ReadOnce(uint64_t addr, uint8_t* buffer, size_t size);
        template <typename T>
        void ReadOnce(uint64_t addr, T& buffer) {
            this->ReadOnce(addr, (uint8_t*)&buffer, sizeof(T));
        }
        

        void PrepareWrite(uint64_t addr, uint8_t* buffer, size_t size);
        template <typename T>
        void PrepareWrite(uint64_t addr, T& buffer) {
            this->PrepareWrite(addr, (uint8_t*)&buffer, sizeof(T));
        }

        // execute write (scatter)
        void Write();

        // writing (single offset)
        void WriteOnce(uint64_t addr, uint8_t* buffer, size_t size);
        template <typename T>
        void WriteOnce(uint64_t addr, T& buffer) {
            this->WriteOnce(addr, (uint8_t*)&buffer, sizeof(T));
        }


        uint64_t GetExport(const std::string& module, const std::string& entry);
        uint64_t GetImport(const std::string& module, const std::string& entry);
        uint64_t GetModuleBase(const std::string& module);
        uint64_t FindPattern(const std::string& module, uint8_t* pattern, const char* mask);
    };
}