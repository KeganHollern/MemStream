//
// Created by Kegan Hollern on 12/24/23.
//
#include <cstdint>
#include <cstdio>

#include "MemStream/Utils.h"

namespace memstream::page {

    // page_head takes any address and returns the first address in the same page on a 64-bit Windows machine
    uint64_t current(uint64_t address) {
        return address & (UINT64_MAX << 12);
    }

    // page_tile takes any address and returns the first address in the next page on a 64-bit Windows machine
    uint64_t next(uint64_t address) {
        return current(address) + 0x1000;
    }

    // offset of an address within it's page
    uint64_t offset(uint64_t address) {
        return address & 0xFFF;
    }
}

namespace memstream::log {
    void buffer(void *buffer, uint32_t size) {
        for (int i = 0; i < size; i++) {
            unsigned char *address = (unsigned char *) (((uint64_t) buffer) + i);

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