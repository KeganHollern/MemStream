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



HRESULT MSS_Push(PMSSDataArray array, uint64_t address, void* buffer, size_t size) {
    if(!array) return E_INVALIDARG;
    if(array->count >= array->capacity) return E_ABORT;
    if(!address) return E_INVALIDARG;
    if(!buffer) return E_INVALIDARG;
    if(!size) return E_INVALIDARG;

    array->addresses[array->count] = address;
    array->buffers[array->count] = buffer;
    array->sizes[array->count] = size;
    array->count++;

    return S_OK;
}

HRESULT MSS_PushMany(PMSSDataArray array, uint64_t addresses[], void* buffers[], size_t sizes[], size_t count) {
    if(!array) return E_INVALIDARG;
    if(array->count >= array->capacity) return E_ABORT;
    if(!addresses) return E_INVALIDARG;
    if(!buffers) return E_INVALIDARG;
    if(!sizes) return E_INVALIDARG;
    if(!count) return E_INVALIDARG;

    HRESULT hr;
    for(int i = 0; i < count; i++) {
        hr = MSS_PushRead(array,  addresses[i], buffers[i], sizes[i]);
        if(FAILED(hr)) return hr;
    }

    return S_OK;
}
HRESULT MSS_FreeArray(PMSSDataArray array) {
    if(!array) return E_INVALIDARG;

    free(array->addresses);
    free(array->buffers);
    free(array->sizes);
    free(array);

    return S_OK;
}
HRESULT MSS_NewReadArray(size_t capacity, PMSSDataArray * pArray) {
    if(!capacity) return E_INVALIDARG;
    if(!pArray) return E_INVALIDARG;

    *pArray = malloc(sizeof(MSSReadArray));
    if(!*pArray) return E_FAIL;

    (*pArray)->sizes = malloc(sizeof(size_t) * capacity);
    (*pArray)->buffers = malloc(sizeof(void*) * capacity);
    (*pArray)->addresses = malloc(sizeof(uint64_t) * capacity);
    (*pArray)->count = 0;
    (*pArray)->capacity = capacity;

    return S_OK;
}

