//
// Created by Kegan Hollern on 12/24/23.
//

#ifndef MEMSTREAM_UTILS_H
#define MEMSTREAM_UTILS_H
namespace memstream::page {
    uint64_t current(uint64_t address);
    uint64_t next(uint64_t address);
    uint64_t offset(uint64_t address);
}
namespace memstream::log {
    void PrintBuffer(void* buffer, uint32_t size);
}
#endif //MEMSTREAM_UTILS_H
