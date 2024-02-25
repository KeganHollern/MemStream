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

        auto current_tick = GetTickCount64();

        for (const auto &offset: this->offsets) {
            const uint32_t addr = offset.first;
            const auto& value = offset.second;

            auto buffer = value.buffer;
            auto size = value.size;

            // drop invalid offsets
            if (!buffer) continue;
            if (size == 0) continue;

            if(value.cache) {
                auto duration = value.cache_duration;
                auto last_cache = offset_caches[addr];


                if(duration == -1) {
                    if(last_cache != 0) continue;
                    duration = 0;
                }

                // check if cache not yet invalidated
                auto cache_invalided_at = last_cache + duration;
                if (current_tick < cache_invalided_at)
                    continue;

                // update last cache (because we're going to push this value)
                offset_caches[addr] = current_tick;
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
        value.cache_duration = 0;

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
        offset value{};
        value.buffer = buffer;
        value.size = size;
        value.cache = true;
        value.cache_duration = cache_duration_ms;

        // TODO: allow_zero for cache

        this->offset_caches[off] = 0;
        this->offsets[off] = value;
    }

    void Object::PushCached(uint32_t off, uint32_t size, uint64_t cache_duration_ms, bool allow_zero) {
        auto buffer = new uint8_t[size]();

        this->PushCachedBuffer(off, buffer, size, cache_duration_ms, allow_zero);
    }

} // dma