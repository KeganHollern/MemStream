//
// Created by Kegan Hollern on 12/24/23.
//

#ifndef MEMSTREAM_UTILS_H
#define MEMSTREAM_UTILS_H

#include "api.h"

namespace memstream::page {
    MEMSTREAM_API uint64_t current(uint64_t address);

    MEMSTREAM_API uint64_t next(uint64_t address);

    MEMSTREAM_API uint64_t offset(uint64_t address);
}
namespace memstream::log {
    MEMSTREAM_API void PrintBuffer(void *buffer, uint32_t size);
}
#endif //MEMSTREAM_UTILS_H
