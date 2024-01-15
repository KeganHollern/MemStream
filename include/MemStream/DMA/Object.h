//
// Created by Kegan Hollern on 12/27/23.
//

#ifndef MEMSTREAM_OBJECT_H
#define MEMSTREAM_OBJECT_H


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

#include <string>
#include <cstring>
#include <cstdint>
#include <tuple>
#include <cassert>
#include <unordered_map>

#include <MemStream/FPGA.h>
#include <MemStream/Process.h>

namespace memstream::dma {

        class MEMSTREAM_API Object {
        public:
            // new object for process
            explicit Object(Process* process);

            // use `-1` for cache duration for inifinite caching
            void PushCachedBuffer(uint32_t off, uint8_t* buffer, uint32_t size, uint64_t cache_duration_ms = 30000, bool allow_zero = true);
            void PushCached(uint32_t off, uint32_t size, uint64_t cache_duration_ms = 30000, bool allow_zero = true);
            // void ResetCache(uint32_t off); // TODO ? how do we invalidate cache...

            //writes
            bool Write();
            void StageWrite();

            // reads
            bool Read();
            void StageRead();

            // is this object NULL (default)
            virtual bool IsNull();

            void PushBuffer(uint32_t off, uint8_t* buffer, uint32_t size);
            void Push(uint32_t off, uint32_t size);

            uint32_t Size(uint32_t off);
            uint8_t* Get(uint32_t off);

            template<typename T>
            inline bool Get(uint32_t off, T& value) {
                auto buff = this->Get(off);
                // buffer null (was this offset pushed?)
                if(!buff) return false;
                // 0 bytes in buffer
                if(!this->Size(off)) return false;
                // cant fit T into N bytes
                if(this->Size(off) < sizeof(value)) return false;

                // we EXPECT we'll always be equal so during DEBUG
                // we'll actually assert this.
                assert(this->Size(off) == sizeof(value) && "unexpect object size");

                // unsafe cast our bytes to T
                std::memcpy(&value, buff, sizeof(T));
                return true;
            }

            // TODO: invalidate all offsets on base change ?!
            uint64_t base;
        protected:
            struct offset {
                uint8_t* buffer;
                uint32_t size;
                bool cache;
                uint64_t last_cache;
                uint64_t cache_duration;
                bool allow_zero_cache;
            };

            std::unordered_map<uint32_t, offset> offsets{};
            Process* proc;
        };

    } // dma

#endif //MEMSTREAM_OBJECT_H
