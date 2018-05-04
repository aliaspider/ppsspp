#pragma once
#include <wiiu/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum shared_data_type_t
{
    SHARED_FONT_CHINESE,
    SHARED_FONT_KOREAN,
    SHARED_FONT_DEFAULT,
    SHARED_FONT_TAIWAN
} shared_data_type_t;

BOOL OSGetSharedData(shared_data_type_t type, uint32_t flags, void **dst, uint32_t *size);

void *OSBlockMove(void *dst, const void *src, uint32_t size, BOOL flush);
void *OSBlockSet(void *dst, uint8_t val, uint32_t size);
uint32_t OSEffectiveToPhysical(void *vaddr);
void *OSAllocFromSystem(uint32_t size, int align);
void OSFreeToSystem(void *ptr);

#define OS_MMAP_INVALID     0
#define OS_MMAP_RO          1
#define OS_MMAP_RW          2
#define OS_MMAP_FREE        3
#define OS_MMAP_ALLOCATED   4
#define OS_MMAP_PAGE_SIZE   0x20000

void* OSAllocVirtAddr(void* vaddr, int size, int align);
BOOL OSFreeVirtAddr(void* vaddr, int size);
int OSQueryVirtAddr(void* vaddr);
void OSGetMapVirtAddrRange(void** vaddr, u32* size);
void OSGetDataPhysAddrRange(u32* paddr, u32* size);
void OSGetAvailPhysAddrRange(u32* paddr, u32* size);
BOOL OSMapMemory(void* vaddr, u32 paddr, int size, int mode);
BOOL OSUnmapMemory(void* vaddr, int size);

void memoryInitialize(void);
void memoryRelease(void);

void * MEM2_alloc(unsigned int size, unsigned int align);
void MEM2_free(void *ptr);

void * MEM1_alloc(unsigned int size, unsigned int align);
void MEM1_free(void *ptr);

void * MEMBucket_alloc(unsigned int size, unsigned int align);
void MEMBucket_free(void *ptr);

#ifdef __cplusplus
}
#endif

/** @} */
