//
// Created by Kegan Hollern on 12/24/23.
//

#ifndef MEMSTREAM_UTILS_H
#define MEMSTREAM_UTILS_H

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

namespace memstream::page {
    MEMSTREAM_API uint64_t current(uint64_t address);

    MEMSTREAM_API uint64_t next(uint64_t address);

    MEMSTREAM_API uint64_t offset(uint64_t address);
}
namespace memstream::log {
    MEMSTREAM_API void buffer(void *buffer, uint32_t size);
}

#endif //MEMSTREAM_UTILS_H
