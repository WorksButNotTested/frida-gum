#include <assert.h>
#include <sys/mman.h>
#define DESTROY_LOCK(l) (0)
#define PAGE_SIZE 4096

typedef void *mspace;

mspace create_mspace(size_t capacity, int locked)
{
    (void)capacity;
    (void)locked;
    return NULL;
}

size_t destroy_mspace(mspace msp)
{
    (void)msp;
    return 0;
}

struct mallinfo mspace_mallinfo(mspace msp)
{
    (void)msp;
    struct mallinfo info = {0};
    return info;
}

size_t mspace_usable_size(const void *mem)
{
    if (!mem)
        return 0;
    const uint8_t *ptr = (const uint8_t *)mem;
    return *(const size_t *)(ptr - sizeof(size_t));
}

void *mspace_memalign(mspace msp, size_t alignment, size_t bytes)
{
    (void)msp;
    size_t aligned_size = (bytes + alignment - 1) & ~(alignment - 1);
    size_t user_size = aligned_size + sizeof(size_t);
    size_t aligned_total_size = (user_size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    size_t total_size = aligned_total_size + 2 * PAGE_SIZE;
    uint8_t *ptr = (uint8_t *)mmap(NULL, total_size, PROT_READ | PROT_WRITE,
                                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED)
        return NULL;
    mprotect(ptr, PAGE_SIZE, PROT_NONE);
    mprotect(&ptr[total_size - PAGE_SIZE], PAGE_SIZE, PROT_NONE);
    uint8_t *buf = &ptr[total_size - PAGE_SIZE - aligned_size];
    *(size_t *)(buf - sizeof(size_t)) = bytes;
    assert (((uintptr_t)buf & (alignment - 1)) == 0);
    assert (mspace_usable_size(buf) == bytes);
    memset (buf, 0, bytes);
    return (void *)buf;
}

void *mspace_malloc(mspace msp, size_t bytes)
{
    return mspace_memalign(msp, 8, bytes);
}

void *mspace_calloc(mspace msp, size_t n_elements, size_t elem_size)
{
    size_t total = n_elements * elem_size;
    void *ptr = mspace_malloc(msp, total);
    if (ptr)
        memset(ptr, 0, total);
    return ptr;
}

void *mspace_realloc(mspace msp, void *oldmem, size_t bytes)
{
    if (!oldmem)
        return mspace_malloc(msp, bytes);
    size_t old_size = mspace_usable_size(oldmem);
    void *newmem = mspace_malloc(msp, bytes);
    if (newmem)
        memcpy(newmem, oldmem, old_size < bytes ? old_size : bytes);
    return newmem;
}

void mspace_free(mspace msp, void *mem)
{
    (void)msp;
    (void)mem;
    size_t bytes = mspace_usable_size(mem);
    uint8_t *page_aligned_start = (uint8_t *)((uintptr_t)mem & ~(PAGE_SIZE - 1));
    uint8_t *page_aligned_end = (uint8_t *)(((uintptr_t)mem + bytes + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1));
    mprotect(page_aligned_start, page_aligned_end - page_aligned_start, PROT_NONE);
}
