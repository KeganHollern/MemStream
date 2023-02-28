#include "memstream.h"

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
}