#pragma once

#include <cstdint>
#include <cstdio>

namespace memstream::v2::log {
    inline void buffer(void *buffer, uint32_t size) {
        for (int i = 0; i < size; i++) {
            auto address = (unsigned char *) (((uint64_t) buffer) + i);

            if (i % 0x10 == 0) {
                if (i != 0) printf("\n");
                printf("0x%04x: ", (unsigned short) (0xFFFF & i));
            } else if (i % 0x8 == 0) {
                printf("- ");
            }
            printf("%02x", 0xFF & *address);
            printf(" ");
        }
    }
}