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

            // caching means we only actually read this value (stage it)
            // once every 30s
            if(value.cache) {
                auto current_tick = GetTickCount64();
                if((current_tick - value.last_cache) <= (30*1000))
                    continue;
                else
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
        this->offsets[off] = {buffer, size, false, 0};
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

    void Object::PushCachedBuffer(uint32_t off, uint8_t *buffer, uint32_t size) {
        this->offsets[off] = {buffer, size, true, 0};
    }

    void Object::PushCached(uint32_t off, uint32_t size) {
        uint8_t* buffer = new uint8_t[size];
        memset(buffer, 0, size);

        this->PushCachedBuffer(off, buffer, size);
    }

} // dma