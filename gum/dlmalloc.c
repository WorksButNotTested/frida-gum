#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define DESTROY_LOCK(l) (0)
#define BUFFER_SIZE (128UL << 20)

typedef void* mspace;

static uint8_t static_buffer[BUFFER_SIZE];
static size_t static_offset = 0;

mspace create_mspace(size_t capacity, int locked) {
    (void)capacity;
    (void)locked;
    static_offset = 0;
    return (mspace)1;
}

size_t destroy_mspace(mspace msp) {
    (void)msp;
    size_t used = static_offset;
    static_offset = 0;
    return used;
}

struct mallinfo mspace_mallinfo(mspace msp) {
    (void)msp;
    struct mallinfo info = {0};
    info.uordblks = (int)static_offset;
    info.fordblks = (int)(BUFFER_SIZE - static_offset);
    return info;
}

void* mspace_malloc(mspace msp, size_t bytes) {
    (void)msp;    
    uint8_t* start = &static_buffer[static_offset] + sizeof(size_t);
    uint8_t* end = start + bytes;
    size_t end_offset = end - static_buffer;
    if (end_offset > BUFFER_SIZE) {
      puts("OOM");
      return NULL;
    }
    *(size_t*)(start - sizeof(size_t)) = bytes;
    static_offset = end_offset;
    return (void*)start;
}

void* mspace_calloc(mspace msp, size_t n_elements, size_t elem_size) {
    size_t total = n_elements * elem_size;
    void* ptr = mspace_malloc(msp, total);
    if (ptr) memset(ptr, 0, total);
    return ptr;
}

size_t mspace_usable_size(const void* mem) {
    if (!mem) return 0;
    const uint8_t* ptr = (const uint8_t*)mem;
    return *(const size_t*)(ptr - sizeof(size_t));
}

void* mspace_realloc(mspace msp, void* oldmem, size_t bytes) {
    if (!oldmem) return mspace_malloc(msp, bytes);
    size_t old_size = mspace_usable_size(oldmem);
    void* newmem = mspace_malloc(msp, bytes);
    if (newmem) memcpy(newmem, oldmem, old_size < bytes ? old_size : bytes);
    return newmem;
}

void* mspace_memalign(mspace msp, size_t alignment, size_t bytes) {
  (void)msp;
  uint8_t* start = &static_buffer[static_offset] + sizeof(size_t);
  uint8_t* aligned = (uint8_t*)(((uintptr_t)(start + alignment - 1)) & ~(uintptr_t)(alignment - 1));
  uint8_t* end = aligned + bytes + sizeof(size_t);
  size_t end_offset = end - static_buffer;
  if (end_offset > BUFFER_SIZE) {
    puts("OOM");
    return NULL;
  }
  *(size_t*)(aligned - sizeof(size_t)) = bytes;
  static_offset = end_offset;
  return (void*)aligned;
}

void mspace_free(mspace msp, void* mem) {
    (void)msp;
    (void)mem;
}
