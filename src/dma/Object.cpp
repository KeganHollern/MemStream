//
// Created by Kegan Hollern on 12/27/23.
//
#include <stdexcept>
#include <cstdint>
#include <cassert>
#include <iostream>

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
        if(this->IsNull()) {
            std::cout << "W: staged null object" << std::endl;
            return;
        }

        if(this->offsets.empty()) {
            std::cout << "W: staged empty object" << std::endl;
            return;
        }

        for (const auto &offset: this->offsets) {
            const uint32_t addr = offset.first;
            const std::tuple<uint8_t*, uint32_t>& value = offset.second;

            uint8_t* buffer = std::get<0>(value);
            uint32_t size = std::get<1>(value);

            if (!buffer) continue;
            if (size == 0) continue;

            this->proc->StageRead(
                    this->base + addr, // BASE+OFF]
                    buffer, // BUFFER of N BYTES
                    size); // N (length of buffer)
        }
    }

    // push an offset to this object structure
    void Object::Push(uint32_t off, uint32_t size) {
        this->PushBuffer(off, new uint8_t[size], size);
    }

    // push an offset to this object structure & store its read data at the buffer
    void Object::PushBuffer(uint32_t off, uint8_t *buffer, uint32_t size) {
        this->offsets[off] = std::make_tuple(buffer, size);
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