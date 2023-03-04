#include "memstream.h"

// #include <stdio.h>
#include <stdint.h>
#include <Windows.h>
#include <leechcore.h>
#include <vmmdll.h>

#include "hashmap.h"

// page_head takes any address and returns the first address in the same page on a 64-bit Windows machine
uint64_t page_head(uint64_t address) {
    return address & (UINT64_MAX << 12);
}
// page_tile takes any address and returns the first address in the next page on a 64-bit Windows machine
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
    return (pa->address - pb->address > 0) ? 1 : -1;
}
uint64_t page_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    const page_read *page = item;
    return hashmap_sip(&page->address, sizeof(uint64_t), seed0, seed1);
}


// extractTargets builds a hashmap of page_read objects
// consisting of unique page addresses that would need to be read to fulfil all reads
struct hashmap* extractTargets(uint64_t* addresses, size_t* sizes, size_t count) {
    if(!addresses) return NULL;
    if(!sizes) return NULL;
    if(!count) return NULL;


    //printf("\tMemStream: constructing hashmap\n");
    struct hashmap* targets = hashmap_new(sizeof(page_read), 0, 0, 0, page_hash, page_compare, NULL, NULL);

    //printf("\tMemStream: iterating reads\n");
    for(size_t j = 0; j < count; j++) {
        uint64_t start = addresses[j]; // inclusive first byte to read
        uint64_t stop = addresses[j] + sizes[j] - 0x1; // inclusive last byte to read

        //printf("\tMemStream: extracting read at 0x%p\n", (void*)start);

        uint64_t start_page = page_head(start); // page containing START byte to read
        uint64_t end_page = page_tail(stop); // first page *NOT* containing END byte to read

        uint64_t num_pages = (end_page - start_page) / 0x1000; // 0x2000 - 0x1000 / 0x1000 = 1 -- 1 page to read starting at start_page
        //printf("\tMemStream: read will start at 0x%p and take %llu pages\n", (void*)start_page, num_pages);


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
            page_read* replaced = hashmap_set(targets, element);
            if(replaced) {
                //printf("\tMemStream: page at 0x%p previously included in targets!\n", (void*)page);
                //free(replaced);
                // TODO: figure out how to free elements otherwise we continue to leak mem :(
            }
        }
    }

    return targets;
}

// parseTargets populates read buffers with content in the hashmap of page data
HRESULT parseTargets(struct hashmap* pages, uint64_t* addresses, void** buffers, size_t* sizes, size_t count) {
    if(!pages) return E_INVALIDARG;
    if(!addresses) return E_INVALIDARG;
    if(!buffers) return E_INVALIDARG;
    if(!sizes) return E_INVALIDARG;
    if(!count) return E_INVALIDARG;

    //printf("\tMemStream: parsing targets...\n");

    for(size_t j = 0; j < count; j++) {
        uint64_t op_address = addresses[j];
        void* op_buffer = buffers[j];
        size_t op_size = sizes[j];

        //printf("\tMemStream: extracting %zu bytes at 0x%p...\n", op_size, (void*)op_address);

        uint64_t start_page = page_head(op_address);
        uint64_t end_page = page_tail(op_address + op_size - 0x1);
        uint64_t num_pages = (end_page - start_page) / 0x1000;

        size_t total_bytes_read = 0;
        for(uint64_t i = 0; i < num_pages; i++) {
            uint64_t page = start_page + (0x1000 * i);
            page_read* read_data = hashmap_get(pages, &(page_read){ .address=page });
            if(!read_data) {
                //printf("\tMemStream: failed to parse page 0x%p...\n", (void*)page);
                ZeroMemory(op_buffer, op_size); // read failure zero output buffer
                continue;
            }
            void* page_buffer = read_data->buffer;

            uint64_t page_read_start = 0; // which byte we want start reading at
            size_t page_read_len = 0x1000; // how many bytes to want to read from this page



            if(i == 0) {
                page_read_start = page_offset(op_address); // start reading page_offset bytes into this first page
                page_read_len -= (size_t)page_read_start; // subtract offset start from total length of read // 0xFB8
            }
            if(i == (num_pages-1)) {
                page_read_len = (op_size - total_bytes_read);
            }

            //printf("\tMemStream: extracting %zu bytes from 0x%p @ 0x%p...\n", page_read_len, (void*)page, (void*)page_read_start);
            // copy data from page buffer to operation buffer
            void* dest_buffer_after_offset = (void*)((uint64_t)op_buffer + total_bytes_read);
            void* source_buffer_after_read_start = (void*)((uint64_t)page_buffer + page_read_start);
            memcpy(dest_buffer_after_offset, source_buffer_after_read_start, page_read_len);


            total_bytes_read += page_read_len;
        }

    }

    //printf("\tMemStream: done extracting targets...\n");
    return S_OK;
}


// MSS_ReadMany will do a single read DMA request to read many addresses
// NOTE: if you read multiple values from the same page, it's more efficient to combine them into a larger buffer.
HRESULT MSS_ReadMany(PMSSProcess process, uint64_t addresses[], void* buffers[], size_t sizes[], size_t count) {
    if(!process || !process->ctx || !process->ctx->hVMM) return E_UNEXPECTED;
    if(!addresses) return E_INVALIDARG;
    if(!buffers) return E_INVALIDARG;
    if(!sizes) return E_INVALIDARG;
    if(!count) return E_INVALIDARG;

    // build a hashmap of all unique pages we'd need to read to fulfill all ops
    //printf("\tMemStream: extracting targets from reads...\n");
    struct hashmap* targets = extractTargets(addresses, sizes, count);
    if(!targets) return E_FAIL;

    size_t num_targets = hashmap_count(targets);
    //printf("\tMemStream: reading %zu targets...\n", num_targets);

    size_t read_size = 0x1000 * num_targets;

    // allocate a single buffer for all reads to write into
    void* read_buffer = malloc(read_size);
    if(!read_buffer) {
        //printf("\tMemStream: failed to allocate read buffer");
        hashmap_free(targets);
        return E_FAIL;
    }

    // allocate scatter structure using our single read_buffer for storage
    PPMEM_SCATTER ppMEMs = NULL;
    if(!LcAllocScatter2(read_size, read_buffer, hashmap_count(targets),&ppMEMs))
    {
        //printf("\tMemStream: failed to allocate scatter buffer");
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
            //printf("\tMemStream: iterator returned null item?");
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
        //printf("\tMemStream: VMM scatter read failed");
        LcMemFree(ppMEMs);
        free(read_buffer);
        hashmap_free(targets);
        return E_FAIL;
    }

    if(FAILED(parseTargets(targets, addresses, buffers, sizes, count))) {
        //printf("\tMemStream: failed to parse targets");
        LcMemFree(ppMEMs);
        free(read_buffer);
        hashmap_free(targets);
        return E_FAIL;
    }

    //printf("\tMemStream: freeing VMM buffer...\n");
    LcMemFree(ppMEMs);
    free(read_buffer);
    hashmap_free(targets);
    return S_OK;
}

HRESULT MSS_PushRead(PMSSReadArray array, uint64_t address, void* buffer, size_t size) {
    if(!array) return E_INVALIDARG;
    if(array->count >= array->capacity) return E_ABORT;
    if(!address) return E_INVALIDARG;
    if(!buffer) return E_INVALIDARG;
    if(!size) return E_INVALIDARG;

    array->read_addresses[array->count] = address;
    array->read_buffers[array->count] = buffer;
    array->read_sizes[array->count] = size;
    array->count++;

    return S_OK;
}

HRESULT MSS_PushManyReads(PMSSReadArray array, uint64_t* addresses, void** buffers, size_t* sizes, size_t count) {
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
HRESULT MSS_FreeRead(PMSSReadArray array) {
    if(!array) return E_INVALIDARG;

    free(array->read_addresses);
    free(array->read_buffers);
    free(array->read_sizes);
    free(array);

    return S_OK;
}
HRESULT MSS_NewReadArray(size_t capacity, PMSSReadArray* pArray) {
    if(!capacity) return E_INVALIDARG;
    if(!pArray) return E_INVALIDARG;

    *pArray = malloc(sizeof(MSSReadArray));
    if(!*pArray) return E_FAIL;

    (*pArray)->read_sizes = malloc(sizeof(size_t) * capacity);
    (*pArray)->read_buffers = malloc(sizeof(void*) * capacity);
    (*pArray)->read_addresses = malloc(sizeof(uint64_t) * capacity);
    (*pArray)->count = 0;
    (*pArray)->capacity = capacity;

    return S_OK;
}
