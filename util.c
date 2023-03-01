#include "memstream.h"

#include <stdio.h>
#include <stdint.h>
#include <Windows.h>
#include <leechcore.h>
#include <vmmdll.h>

typedef struct msslist {
    struct msslist* next;
    void* buffer;
} msslist;

HRESULT MSS_Free(void* list) {
    if(!list) return E_INVALIDARG;

    msslist* current = (msslist*)list;

    do {
        free(current->buffer); // free allocated value buffer
        // free this element & nav down the list
        msslist* next = current->next;
        free(current);
        current = next;
    } while(current);

    return S_OK;
}

void MSS_PrintBuffer(void* buffer, size_t size) {
    for(int i = 0; i < size; i++) {
        unsigned char* address = (unsigned char*)(((uint64_t)buffer) + i);


        if(i % 0x10 == 0) {
            if(i != 0) printf("\n");
            printf("0x%04x: ", (unsigned short)(0xFFFF & i));
        } else if(i % 0x8 == 0) {
            printf("- ");
        }
        printf("%02x", 0xFF & *address);
        printf(" ");
    }
}