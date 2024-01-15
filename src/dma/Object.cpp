#include <stdexcept>
#include <cassert>

#include "MemStream/DMA/Object.h"

namespace memstream::dma {
    Object::Object(Process *process) {
        if (!process)
            throw std::invalid_argument("null process");

        this->base = 0x0;
        this->proc = process;
        this->offsets.clear();
    }

    bool Object::IsNull() {
        return this->base == 0x0;
    }

    bool Object::Read() {
        assert(this->proc && "null proc");

        if(this->IsNull()) return true;
        if (this->offsets.empty()) return true;

        this->StageRead(); // stage offsets for read
        return this->proc->ExecuteStagedReads(); // read
    }


    bool Object::Write() {
        assert(this->proc && "null proc");

        if(this->IsNull()) return true;
        if (this->offsets.empty()) return true;

        this->StageWrite(); // stage offsets for read
        return this->proc->ExecuteStagedWrites(); // read
    }
    void Object::StageWrite() {
        assert(this->proc && "null proc");

        // can't read if null...
        if(this->IsNull()) return;
        if(this->offsets.empty()) return;

        for (auto &offset: this->offsets) {
            const uint32_t addr = offset.first;
            auto& value = offset.second;

            uint8_t* buffer = value.buffer;
            uint32_t size = value.size;

            if (!buffer) continue;
            if (size == 0) continue;

            this->proc->StageWrite(
                    this->base + addr, // BASE+OFF]
                    buffer, // BUFFER of N BYTES
                    size); // N (length of buffer)
        }
    }


    void Object::StageRead() {
        assert(this->proc && "null proc");

        // can't read if null...
        if(this->IsNull()) return;

        if(this->offsets.empty()) return;

        for (auto &offset: this->offsets) {
            const uint32_t addr = offset.first;
            auto& value = offset.second;

            uint8_t* buffer = value.buffer;
            uint32_t size = value.size;

            if (!buffer) continue;
            if (size == 0) continue;

            if(value.cache) {
                // CACHING
                auto current_tick = GetTickCount64();
                if(value.cache_duration == -1) {
                    // NO RECACHING
                    if(value.allow_zero_cache && value.last_cache != 0)
                        continue; // ALREADY CACHED ONCE

                    bool is_zero = true;
                    for(int i = 0; i < size; i++) {
                        if(buffer[i]) { is_zero = false; break; }
                    }
                    if(!is_zero)
                        continue; // ALREADY CACHED NONZERO VALUE
                } else {
                    // RECACHING ENABLED
                    if ((current_tick - value.last_cache) <= value.cache_duration)
                        continue; // NOT TIME TO RECACHE
                }

                // if we made it here we're recaching (or caching) and thus
                // need to update last_cache
                value.last_cache = current_tick;
            }

            this->proc->StageRead(
                    this->base + addr, // BASE+OFF]
                    buffer, // BUFFER of N BYTES
                    size); // N (length of buffer)
        }
    }

    // push an offset to this object structure
    void Object::Push(uint32_t off, uint32_t size) {
        uint8_t* buffer = new uint8_t[size];
        memset(buffer, 0, size);

        this->PushBuffer(off, buffer, size);
    }

    // push an offset to this object structure & store its read data at the buffer
    void Object::PushBuffer(uint32_t off, uint8_t *buffer, uint32_t size) {
        this->offsets[off] = {buffer, size, false, 0, 0};
    }

    // get the read value from the object structure
    uint8_t *Object::Get(uint32_t off) {
        auto itr = this->offsets.find(off);
        if (itr != this->offsets.end()) {
            return itr->second.buffer;
        }
        
        return nullptr;
    }

    uint32_t Object::Size(uint32_t off) {
        auto itr = this->offsets.find(off);
        if (itr != this->offsets.end()) {
            return itr->second.size;
        }
        return 0;
    }

    void Object::PushCachedBuffer(uint32_t off, uint8_t *buffer, uint32_t size, uint64_t cache_duration_ms, bool allow_zero) {
        this->offsets[off] = {buffer, size, true, 0, cache_duration_ms, allow_zero};
    }

    void Object::PushCached(uint32_t off, uint32_t size, uint64_t cache_duration_ms, bool allow_zero) {
        uint8_t* buffer = new uint8_t[size];
        memset(buffer, 0, size);

        this->PushCachedBuffer(off, buffer, size, cache_duration_ms, allow_zero);
    }

} // dma