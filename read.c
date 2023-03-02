#include "memstream.h"

#include <stdint.h>
#include <Windows.h>
#include <leechcore.h>
#include <vmmdll.h>

#include "hashmap.h"

// page_head takes any address and returns the first address in the same page on a 64 bit windows machine
uint64_t page_head(uint64_t address) {
    return address & (UINT64_MAX << 12);
}
// page_tile takes any address and returns the first address in the next page on a 64 bit windows machine
uint64_t page_tail(uint64_t address) {
    return page_head(address) + 0x1000;
}
uint64_t page_offset(uint64_t address) {
    return address & 0xFFF;
}

ULONG64 MSS_READ_FLAGS = VMMDLL_FLAG_NO_PREDICTIVE_READ | VMMDLL_FLAG_NOCACHE | VMMDLL_FLAG_NOPAGING | VMMDLL_FLAG_NOCACHEPUT;





// MSS_ReadSingle reads a single address into a buffer.
HRESULT MSS_ReadSingle(PMSSProcess process, uint64_t address, void* buffer, size_t size) {
    if(!process || !process->ctx || !process->ctx->hVMM) return E_UNEXPECTED;
    if(!buffer) return E_INVALIDARG;
    if(!size) return E_INVALIDARG;

    DWORD read = 0;
    if(!VMMDLL_MemReadEx(
            process->ctx->hVMM,
            process->pid,
            address,
            buffer,
            size,
            &read,
            MSS_READ_FLAGS))
        return E_FAIL;

    return S_OK;
}

// --- structures help with scatter read hashmap logic

typedef struct page_read {
    uint64_t address; // page address
    void* buffer; // page sized buffer
} page_read;
int page_compare(const void* a, const void* b, void* udata) {
    const page_read *pa = a;
    const page_read *pb = b;
    if(pa->address == pb->address) return 0;
    return (pa->address-pb->address > 0) ? 1 : -1;
}
uint64_t page_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    const page_read *page = item;
    return hashmap_sip(&page->address, sizeof(uint64_t), seed0, seed1);
}


// extractTargets builds a hashmap of page_read objects
// consisting of unique page addresses that would need to be read to fulfil all reads
struct hashmap* extractTargets(ReadOp* reads) {
    if(!reads) return NULL;

    ReadOp* current = reads;

    struct hashmap* targets = hashmap_new(sizeof(page_read), 0, 0, 0, page_hash, page_compare, NULL, NULL);

    while (current) {
        uint64_t start = current->address; // inclusive first byte to read
        uint64_t stop = current->address + current->size - 0x1; // inclusive last byte to read

        uint64_t start_page = page_head(start); // page containing START byte to read
        uint64_t end_page = page_tail(stop); // first page *NOT* containing END byte to read

        uint64_t num_pages = (end_page - start_page) / 0x1000; // 0x2000 - 0x1000 / 0x1000 = 1 -- 1 page to read starting at start_page

        for(uint64_t i = 0; i < num_pages; i++) {
            uint64_t page = start_page + (0x1000 * i);

            // insert pages into hashmap w/ NULL buffer pointers
            page_read* element = malloc(sizeof(page_read));
            if(!element) {
                hashmap_free(targets);
                return NULL;
            }
            element->address = page;
            element->buffer = NULL;
            hashmap_set(targets, element);
        }

        current=current->next;
    }

    return targets;
}

// parseTargets populates read buffers with content in the hashmap of page data
HRESULT parseTargets(ReadOp* reads, struct hashmap* pages) {
    if(!reads) return E_INVALIDARG;
    if(!pages) return E_INVALIDARG;

    ReadOp* current = reads;

    while(current) {
        uint64_t op_address = current->address;
        void* op_buffer = current->buffer;
        size_t op_size = current->size;

        uint64_t start_page = page_head(op_address);
        uint64_t end_page = page_tail(op_address + op_size - 0x1);
        uint64_t num_pages = (end_page - start_page) / 0x1000;

        size_t total_bytes_read = 0;
        for(uint64_t i = 0; i < num_pages; i++) {
            uint64_t page = start_page + (0x1000 * i);
            page_read* read_data = hashmap_get(pages, &(page_read){ .address=page });
            if(!read_data) {
                ZeroMemory(op_buffer, op_size); // read failure zero output buffer
                continue;
            }
            void* page_buffer = read_data->buffer;

            uint64_t page_read_start = 0; // which byte we want start reading at
            size_t page_read_len = 0x1000; // how many bytes to want to read from this page

            if(i == 0) {
                page_read_start = page_offset(op_address); // start reading page_offset bytes into this first page
                page_read_len -= page_read_start; // subtract offset start from total length of read
            }
            if(i == (num_pages-1)) {
                // last page - length is not full page
                page_read_len -= op_size - total_bytes_read;
            }

            // copy data from page buffer to operation buffer
            void* dest_buffer_after_offset = (void*)((uint64_t)op_buffer + total_bytes_read);
            void* source_buffer_after_read_start = (void*)((uint64_t)page_buffer + page_read_start);
            memcpy(dest_buffer_after_offset, source_buffer_after_read_start, page_read_len);


            total_bytes_read += page_read_len;
        }

        current=current->next;
    }

    return S_OK;
}


// MSS_ReadMany will do a single read DMA request to read many addresses
// NOTE: if you read multiple values from the same page, it's more efficient to combine them into a larger buffer.
HRESULT MSS_ReadMany(PMSSProcess process, ReadOp* reads) {
    if(!process || !process->ctx || !process->ctx->hVMM) return E_UNEXPECTED;
    if(!reads) return E_INVALIDARG;

    // build a hashmap of all unique pages we'd need to read to fulfill all ops
    struct hashmap* targets = extractTargets(reads);
    if(!targets) return E_FAIL;

    size_t read_size = 0x1000 * hashmap_count(targets);

    // allocate a single buffer for all reads to write into
    void* read_buffer = malloc(read_size);
    if(!read_buffer) {
        hashmap_free(targets);
        return E_FAIL;
    }

    // allocate scatter structure using our single read_buffer for storage
    PPMEM_SCATTER ppMEMs = NULL;
    if(!LcAllocScatter2(read_size, read_buffer, hashmap_count(targets),&ppMEMs))
    {
        free(read_buffer);
        hashmap_free(targets);
        return E_FAIL;
    }

    // link scatter and hashmap pointers
    size_t iter = 0;
    void *item;
    int idx = 0;
    while (hashmap_iter(targets, &iter, &item)) {
        page_read *page = item;
        if(!page) {
            continue;
        }
        page->buffer = ppMEMs[idx]->pb; // point page buffer to the same region ppMEMs writes to
        ppMEMs[idx]->qwA = page->address; // point ppMEMs read address to the same as the page expects
        idx++;
    }

    // at this point a memreadscatter should populate read_buffer and the contents of each page read should be accessable via targets map
    if (!VMMDLL_MemReadScatter(
            process->ctx->hVMM,
            process->pid,
            ppMEMs,
            hashmap_count(targets),
            MSS_READ_FLAGS | VMMDLL_FLAG_ZEROPAD_ON_FAIL)) {
        LocalFree(ppMEMs);
        free(read_buffer);
        hashmap_free(targets);
        return E_FAIL;
    }

    if(FAILED(parseTargets(reads, targets))) {
        LocalFree(ppMEMs);
        free(read_buffer);
        hashmap_free(targets);
        return E_FAIL;
    }

    LocalFree(ppMEMs);
    free(read_buffer);
    hashmap_free(targets);
    return S_OK;
}

// MSS_NewReadOp constructs a read operation for use with MSS_ReadMany
HRESULT MSS_NewReadOp(uint64_t address, void* buffer, size_t size, PReadOp* pReadOp) {
    if(!address) return E_INVALIDARG;
    if(!buffer) return E_INVALIDARG;
    if(!size) return E_INVALIDARG;
    if(!pReadOp) return E_INVALIDARG;

    *pReadOp = (PReadOp)malloc(sizeof(ReadOp));
    if(!*pReadOp) return E_FAIL;

    (*pReadOp)->address = address;
    (*pReadOp)->size = size;
    (*pReadOp)->buffer = buffer;
    // no links
    (*pReadOp)->next = NULL;
    (*pReadOp)->prev = NULL;

    return S_OK;
}

// MSS_FreeReadOp releases the read operation - stitching back together any linked list pointers
HRESULT MSS_FreeReadOp(PReadOp read) {
    if(!read) return E_INVALIDARG;
    // relink list depending on any links set in read we're freeing
    if(read->prev && read->prev->next == read && read->next && read->next->prev == read) {
       ReadOp* next = read->next;
       ReadOp* prev = read->prev;

       prev->next = next;
       next->prev = prev;
    } else if(read->prev && read->prev->next == read) {
        read->prev->next = NULL;
    } else if(read->next && read->next->prev == read) {
        read->next->prev = NULL;
    }

    free(read);

    return S_OK;
}

// MSS_InsertReadOp inserts the read operation after the parent operation
//  ex: PARENT<->CHILD => PARENT<->READ<->CHILD
HRESULT MSS_InsertReadOp(PReadOp parent, PReadOp read) {
    if(!parent) return E_INVALIDARG;
    if(!read) return E_INVALIDARG;

    read->prev = parent;
    read->next = parent->next;
    parent->next = read;

    return S_OK;
}

// MSS_CreateReadOps will construct a read op linked list from a list of address, buffers, and read sizes
HRESULT MSS_CreateReadOps(uint64_t addresses[], void* buffers[], size_t sizes[], size_t count, PReadOp* pReads) {
    if(!addresses) return E_INVALIDARG;
    if(!buffers) return E_INVALIDARG;
    if(!sizes) return E_INVALIDARG;
    if(count == 0) return E_INVALIDARG;
    if(!pReads) return E_INVALIDARG;

    HRESULT hr;

    ReadOp* current = NULL;

    for(int i = 0; i < count; i++) {
        uint64_t address = addresses[i];
        void* buffer = buffers[i];
        size_t size = sizes[i];

        PReadOp new = NULL;

        hr = MSS_NewReadOp(address, buffer, size, &new);
        if(FAILED(hr)) return hr;

        new->next = NULL;
        new->prev = current;

        if(current) {
            current->next = new;
        } else {
            *pReads = new;
        }

        current = new;
    }

    return S_OK;
}

