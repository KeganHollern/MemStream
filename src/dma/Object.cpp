//
// Created by Kegan Hollern on 12/27/23.
//
#include <stdexcept>
#include <cstdint>
#include <cassert>

#include "MemStream/DMA/Object.h"

namespace memstream::dma {
    Object::Object(Process *process) {
        if (!process)
            throw std::invalid_argument("null process");

        this->base = 0x0;
        this->proc = process;
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

        for (auto &offset: this->offsets) {
            assert(std::get<0>(offset.second) && "null buffer in offsets");

            if (!std::get<0>(offset.second)) continue;

            this->proc->StageRead(
                    this->base + offset.first, // BASE+OFF]
                    std::get<0>(offset.second), // BUFFER of N BYTES
                    std::get<1>(offset.second)); // N (length of buffer)
        }
    }

    // push an offset to this object structure
    void Object::Push(uint32_t off, uint32_t size) {
        // i _really_ hate that i alloc to heap like this.
        // i should make a PushMany() or something which can do all this
        // in a single allocation....
        this->offsets[off] = std::tuple<uint8_t *, uint32_t>(new uint8_t[size], size);
    }

    // get the read value from the object structure
    uint8_t *Object::Get(uint32_t off) {
        auto itr = this->offsets.find(off);
        if (itr != this->offsets.end()) {
            return std::get<0>(itr->second);
        }
        
        return nullptr;
    }

    uint32_t Object::Size(uint32_t off) {
        auto itr = this->offsets.find(off);
        if (itr != this->offsets.end()) {
            return std::get<1>(itr->second);
        }
        return 0;
    }

} // dma