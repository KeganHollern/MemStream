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
#include <tuple>
#include <string>
#include <list>
#include <memory>

#include <vmmdll.h>
#include <MemStream/FPGA.h>

namespace memstream {
    class MEMSTREAM_API ScatterOp {
    public:
        ScatterOp(uint64_t address, uint32_t size);
        ScatterOp(uint64_t address, uint8_t* buffer, uint32_t size);
        ~ScatterOp();

        inline bool Valid() const {
            return address && buffer && size;
        }

        inline bool PrepareRead(VMMDLL_SCATTER_HANDLE hScatter) {
            return VMMDLL_Scatter_PrepareEx(hScatter, this->address, this->size, this->buffer, (PDWORD)&this->cbRead);
        }
        inline bool PrepareWrite(VMMDLL_SCATTER_HANDLE hScatter) {
            return VMMDLL_Scatter_PrepareWriteEx(hScatter, this->address, this->buffer, this->size);
        }

        uint64_t address = 0;
        uint8_t* buffer = nullptr;
        uint32_t size = 0;
        // TODO: update this var for usage in writes...
        uint32_t cbRead = 0;
    private:
        bool allocated = false;
    };


    class MEMSTREAM_API Process {
    public:
        explicit Process(uint32_t pid);

        explicit Process(const std::string &name);

        Process(FPGA *pFPGA, uint32_t pid);

        Process(FPGA *pFPGA, const std::string &name);

        virtual ~Process();

        // reads

        bool Read(uint64_t addr, uint8_t *buffer, uint32_t size);

        template<typename T>
        inline bool Read(uint64_t addr, T &value) {
            return Read(addr, (uint8_t*)(&value), sizeof(T));
        }


        void StageRead(uint64_t addr, uint8_t *buffer, uint32_t size);

        template<typename T>
        inline void StageRead(uint64_t addr, T &value) {
            StageRead(addr, (uint8_t *)(&value), sizeof(T));
        }

        std::list<uint64_t> ExecuteStagedReads();

         bool ReadMany(std::list<std::shared_ptr<ScatterOp>> &reads);

        // writes

         bool Write(uint64_t addr, uint8_t *buffer, uint32_t size);

        template<typename T>
        inline bool Write(uint64_t addr, T &value) {
            return Write(addr, (uint8_t *)(&value), sizeof(T));
        }


         void StageWrite(uint64_t addr, uint8_t *buffer, uint32_t size);

        template<typename T>
        inline void StageWrite(uint64_t addr, T &value) {
            StageWrite(addr, (uint8_t *)(&value), sizeof(T));
        }

         bool ExecuteStagedWrites();

         bool WriteMany(std::list<std::shared_ptr<ScatterOp>> &writeOps);

        // info stuff

         uint64_t GetModuleBase(const std::string &name);

         bool GetModuleInfo(const std::string &name, VMMDLL_MAP_MODULEENTRY &info);

        // map getters

         std::vector<VMMDLL_MAP_EATENTRY> GetExports(const std::string &name);

         std::vector<VMMDLL_MAP_IATENTRY> GetImports(const std::string &name);

         std::vector<VMMDLL_MAP_MODULEENTRY> GetModules();

         std::vector<VMMDLL_MAP_THREADENTRY> GetThreads();

        // easier to access import/export lookups

         uint64_t GetExport(const std::string &moduleName, const std::string &exportName);

         uint64_t GetImport(const std::string &moduleName, const std::string &importName);

        // utils for cheating / exploiting

         uint64_t FindPattern(uint64_t start, uint64_t stop, const std::string &pattern);

        // The two below are very windows-specific
        // TODO: move them into a windows specific process child class ?
        // EXECUTE() would also go there....

         uint64_t Cave(const std::string &moduleName, uint32_t size);

         bool Dump(const std::string &path);

        // Execute(fnc, args...) rax
        // Hook ?
        bool isIs64Bit() const;

        uint32_t getPid() const;
        const char* getName() const;


        void setVerbose(bool verbose);

    protected:
        VMMDLL_PROCESS_INFORMATION info;
        VMMDLL_SCATTER_HANDLE scatter;
        FPGA *pFPGA;

        uint32_t pid{};



        std::list<std::shared_ptr<ScatterOp>> stagedReads;
        std::list<std::shared_ptr<ScatterOp>> stagedWrites;

        static std::vector<std::tuple<uint8_t, bool>> parsePattern(const std::string &pattern);

        bool verbose = false;
    };


    

} // memstream

#endif //MEMSTREAM_PROCESS_H
