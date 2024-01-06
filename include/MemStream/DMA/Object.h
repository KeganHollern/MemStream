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


            void PushCachedBuffer(uint32_t off, uint8_t* buffer, uint32_t size);
            void PushCached(uint32_t off, uint32_t size);
            // void ResetCache(uint32_t off); // TODO ? how do we invalidate cache...

            // immediate read of object values
            bool Read();

            // stage read of object value
            void StageRead();

            // is this object NULL (default)
            virtual bool IsNull();

            void PushBuffer(uint32_t off, uint8_t* buffer, uint32_t size);
            // push offsets into the object to be read during READ() ops ?
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

            uint64_t base;
        protected:
            struct offset {
                uint8_t* buffer;
                uint32_t size;
                bool cache;
                ULONGLONG last_cache;
            };

            std::unordered_map<uint32_t, offset> offsets{};
            Process* proc;
        };

    } // dma

#endif //MEMSTREAM_OBJECT_H
