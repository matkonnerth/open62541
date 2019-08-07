#include "memory.h"
#include <stdint.h>
#include <stdio.h>
#include <open62541/architecture_functions.h>


struct RawMem
{
    struct RawMem *prev;
    void *mem;
};

struct MemoryPool {
    size_t elementSize;
    size_t incrementCount;
    size_t size;
    size_t maxSize;
    struct RawMem* mem;
};

struct MemoryPool*
MemoryPool_init(size_t elementSize, size_t incrementingSize)
{
    struct MemoryPool* memPool = (struct MemoryPool*) UA_malloc(sizeof(struct MemoryPool));
    if(!memPool)
    {
        return NULL;
    }
    memPool->elementSize = elementSize;
    memPool->size = 0;
    memPool->incrementCount = incrementingSize;
    memPool->maxSize = incrementingSize;
    memPool->elementSize = elementSize;
    memPool->mem = (struct RawMem*) malloc(sizeof(struct RawMem));
    memPool->mem->mem = UA_calloc(memPool->elementSize, memPool->incrementCount);
    if(!memPool->mem->mem)
    {
        return NULL;
    }
    memPool->mem->prev = NULL;    
    return memPool;
}

void *
MemoryPool_getMemoryForElement(struct MemoryPool *memPool)
{
    if(memPool->size >= memPool->maxSize)
    {
        struct RawMem* newRawMem = (struct RawMem*) UA_malloc(sizeof(struct RawMem));
        if(!newRawMem)
        {
            return NULL;
        }
        newRawMem->prev = memPool->mem;
        newRawMem->mem = UA_calloc(memPool->elementSize, memPool->incrementCount);
        if(!newRawMem->mem)
        {
            return NULL;
        }
        memPool->size = 0;
        memPool->mem = newRawMem;
    }
    uintptr_t addr = (uintptr_t)memPool->mem->mem + memPool->elementSize * memPool->size;
    memPool->size++;
    return (void *)addr;
}

void MemoryPool_cleanup(struct MemoryPool *memPool)
{
    while(memPool->mem) {
        UA_free(memPool->mem->mem);
        struct RawMem *nextToFree = memPool->mem->prev;
        UA_free(memPool->mem);
        memPool->mem = nextToFree;
    }
    UA_free(memPool);
}

void
MemoryPool_forEach(const struct MemoryPool *memPool, void (*f)(void *element, void *data),
                   void *data)
{
    struct RawMem *actMem = memPool->mem;
    while(actMem)
    {
        for(size_t cnt = 0; cnt < memPool->size; cnt++)
        {
            uintptr_t adr = (uintptr_t)actMem->mem + memPool->elementSize * cnt;
            f((void *)adr, data);
        }
        actMem = actMem->prev;
    }
}
