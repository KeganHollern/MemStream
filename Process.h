//
// Created by Kegan Hollern on 12/23/23.
//

#ifndef MEMSTREAM_PROCESS_H
#define MEMSTREAM_PROCESS_H

namespace memstream {

    class Process {
    public:
        Process(FPGA* pFPGA, uint64_t pid);
        Process(FPGA* pFPGA, std::string name);

        virtual ~Process();

        virtual bool Read(uint64_t addr, void* buffer, uint32_t size);
        virtual bool Write(uint64_t addr, void* buffer, uint32_t size);
        // ModuleInfo(name) info_t
        // Exports(name) export_t[]
        // Imports(name) import_t[]

        virtual uint64_t FindPattern(
                uint64_t start,
                uint64_t stop,
                uint8_t* pattern,
                uint8_t* mask);
        virtual uint64_t FindCave(
                uint64_t start,
                uint64_t stop);
        // Dump(disk_path)
        // Execute(fnc, args...) rax
        // Hook ?
        // Threads ?

    private:
        bool is64bit;
        uint64_t pid;
    public:
        bool isIs64Bit() const;

        uint64_t getPid() const;
    };

} // memstream

#endif //MEMSTREAM_PROCESS_H
