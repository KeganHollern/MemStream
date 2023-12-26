//
// Created by Kegan Hollern on 12/23/23.
//

#ifndef MEMSTREAM_PROCESS_H
#define MEMSTREAM_PROCESS_H

#if defined(_WIN32)
#if defined(MEMSTREAM_EXPORTS)
#define MEMSTREAM_API __declspec(dllexport)
#else
#define MEMSTREAM_API __declspec(dllimport)
#endif
#else
#if __GNUC__ >= 4
#define MEMSTREAM_API __attribute__ ((visibility ("default")))
#else
#define MEMSTREAM_API
#endif
#endif

#ifndef IMAGE_SCN_MEM_EXECUTE
#define IMAGE_SCN_MEM_EXECUTE   0x20000000
#define IMAGE_SCN_MEM_READ      0x40000000
#define IMAGE_SCN_MEM_WRITE     0x80000000
#endif

#include <cstdint>
#include <vector>
#include <tuple>
#include <string>

#include <vmmdll.h>
#include <MemStream/FPGA.h>

namespace memstream {

    class MEMSTREAM_API Process {
    public:
        explicit Process(uint32_t pid);

        explicit Process(const std::string &name);

        Process(FPGA *pFPGA, uint32_t pid);

        Process(FPGA *pFPGA, const std::string &name);

        virtual ~Process();

        // reads

        virtual bool Read(uint64_t addr, uint8_t *buffer, uint32_t size);

        template<typename T>
        inline bool Read(uint64_t addr, T &value) {
            return Read(addr, reinterpret_cast<uint8_t *>(&value), sizeof(T));
        }


        virtual void StageRead(uint64_t addr, uint8_t *buffer, uint32_t size);

        template<typename T>
        inline void StageRead(uint64_t addr, T &value) {
            StageRead(addr, reinterpret_cast<uint8_t *>(&value), sizeof(T));
        }

        virtual bool ExecuteStagedReads();

        virtual bool ReadMany(std::vector<std::tuple<uint64_t, uint8_t *, uint32_t>> &readOps);

        // writes

        virtual bool Write(uint64_t addr, uint8_t *buffer, uint32_t size);

        template<typename T>
        inline bool Write(uint64_t addr, T &value) {
            return Write(addr, reinterpret_cast<uint8_t *>(&value), sizeof(T));
        }


        virtual void StageWrite(uint64_t addr, uint8_t *buffer, uint32_t size);

        template<typename T>
        inline void StageWrite(uint64_t addr, T &value) {
            StageWrite(addr, reinterpret_cast<uint8_t *>(&value), sizeof(T));
        }

        virtual bool ExecuteStagedWrites();

        virtual bool WriteMany(std::vector<std::tuple<uint64_t, uint8_t *, uint32_t>> &readOps);

        // info stuff

        virtual uint64_t GetModuleBase(const std::string &name);

        virtual bool GetModuleInfo(const std::string &name, VMMDLL_MAP_MODULEENTRY &info);

        // map getters

        virtual std::vector<VMMDLL_MAP_EATENTRY> GetExports(const std::string &name);

        virtual std::vector<VMMDLL_MAP_IATENTRY> GetImports(const std::string &name);

        virtual std::vector<VMMDLL_MAP_MODULEENTRY> GetModules();

        virtual std::vector<VMMDLL_MAP_THREADENTRY> GetThreads();

        // easier to access import/export lookups

        virtual uint64_t GetExport(const std::string &moduleName, const std::string &exportName);

        virtual uint64_t GetImport(const std::string &moduleName, const std::string &importName);

        // utils for cheating / exploiting

        virtual uint64_t FindPattern(uint64_t start, uint64_t stop, const std::string &pattern);

        // The two below are very windows-specific
        // TODO: move them into a windows specific process child class ?
        // EXECUTE() would also go there....

        virtual uint64_t Cave(const std::string &moduleName, uint32_t size);

        virtual bool Dump(const std::string &path);

        // Execute(fnc, args...) rax
        // Hook ?

    private:
        VMMDLL_PROCESS_INFORMATION info;
        FPGA *pFPGA;


        std::vector<std::tuple<uint64_t, uint8_t *, uint32_t>> stagedReads;
        std::vector<std::tuple<uint64_t, uint8_t *, uint32_t>> stagedWrites;

        static std::vector<std::tuple<uint8_t, bool>> parsePattern(const std::string &pattern);

    public:
        bool isIs64Bit() const;

        uint32_t getPid() const;
    };

} // memstream

#endif //MEMSTREAM_PROCESS_H
