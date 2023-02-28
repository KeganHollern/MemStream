#include "memstream.h"

#include <stdint.h>
#include <Windows.h>
#include <leechcore.h>
#include <vmmdll.h>


typedef struct msslist {
    struct msslist* next;
    enum MSS_LIST type;
    void* buffer;
} msslist;

HRESULT MSS_Free(void* list) {
    if(!list) return E_INVALIDARG;

    msslist* current = (msslist*)list;
    do {
        switch(current->type) {
            case MSS_LIST_EXPORTS:
            case MSS_LIST_IMPORTS:
                free(current->buffer); // free buffer (char* NAME)
                break;
            case MSS_LIST_PID:
                // nothing to free
                break;
            default:
                return E_UNEXPECTED;
        }
        // free this element & nav down the list
        msslist* next = current->next;
        free(current);
        current = next;
    } while(current);
}