#include <stdexcept>

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
        if(this->IsNull()) return true;
        if (this->offsets.empty()) return true;

        this->StageRead(); // stage offsets for read
        return this->proc->ExecuteStagedReads(); // read
    }


    bool Object::Write() {
        if(this->IsNull()) return true;
        if (this->offsets.empty()) return true;

        this->StageWrite(); // stage offsets for read
        return this->proc->ExecuteStagedWrites(); // read
    }
    void Object::StageWrite() {
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
        // can't read if null...
        if(this->IsNull()) return;

        if(this->offsets.empty()) return;

        for (auto &offset: this->offsets) {
            const uint32_t addr = offset.first;
            auto& value = offset.second;

            uint8_t* buffer = value.buffer;
            uint32_t size = value.size;

            // drop invalid offsets
            if (!buffer) continue;
            if (size == 0) continue;

            if(value.cache) { // CACHED
                auto current_tick = GetTickCount64();
                auto duration = value.cache_duration;

                if(value.cache_duration == -1 && value.allow_zero_cache && value.last_cache != 0) {
                    // ALREADY CACHED ONCE
                    // no more reads necessary
                    continue;
                } else if(value.cache_duration == -1) {

                    // if we already know there is a non-zero value then we
                    // can just continue
                    if(value.value_non_zero)
                        continue;

                    // look for non-zero value
                    for(uint32_t i = 0; i < size; i++) {
                        if(buffer[i]) {
                            value.value_non_zero = true;
                            break;
                        }
                    }

                    // we found a non-zero byte in buffer so
                    // we can just continue
                    if(value.value_non_zero)
                        continue;

                    // zero values will be read every second until a non-zero is found
                    // TODO: make this configurable by adjusting the NEGATIVE in cache_duration ?
                    duration = 1000;
                }

                // check if cache not yet invalidated
                auto cache_invalided_at = value.last_cache + duration;
                if (current_tick < cache_invalided_at)
                    continue;

                // update last cache (because we're going to push this value)
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
        auto buffer = new uint8_t[size]();

        this->PushBuffer(off, buffer, size);
    }

    // push an offset to this object structure & store its read data at the buffer
    void Object::PushBuffer(uint32_t off, uint8_t *buffer, uint32_t size) {
        offset value{};
        value.buffer = buffer;
        value.size = size;
        value.cache = false;
        value.last_cache = 0;
        value.cache_duration = 0;
        value.allow_zero_cache = false;
        value.value_non_zero = false;

        this->offsets[off] = value;
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
        // TODO: remove
        // TEMPORARY FIX
        if(cache_duration_ms == -1)
            cache_duration_ms = 60*1000;

        offset value{};
        value.buffer = buffer;
        value.size = size;
        value.cache = true;
        value.last_cache = 0;
        value.cache_duration = cache_duration_ms;
        value.allow_zero_cache = allow_zero;
        value.value_non_zero = false;

        this->offsets[off] = value;
    }

    void Object::PushCached(uint32_t off, uint32_t size, uint64_t cache_duration_ms, bool allow_zero) {
        auto buffer = new uint8_t[size]();

        this->PushCachedBuffer(off, buffer, size, cache_duration_ms, allow_zero);
    }

} // dma