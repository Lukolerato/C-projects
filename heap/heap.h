#ifndef ALOKATOR_HEAP_H
#define ALOKATOR_HEAP_H
#define FENCE 4
#include <stdio.h>

enum pointer_type_t
{
    pointer_null,
    pointer_heap_corrupted,
    pointer_control_block,
    pointer_inside_fences,
    pointer_inside_data_block,
    pointer_unallocated,
    pointer_valid
};

struct memory_manager_t
{
    void *memory_start;
    size_t memory_size;
    struct memory_chunk_t *first_memory_chunk;
    size_t flag;
};

struct __attribute__((packed)) memory_chunk_t
{
    struct memory_chunk_t* prev;
    struct memory_chunk_t* next;
    size_t size;
    size_t waste;
    int free; // 1-wolny, 0-zajety
    int checksum;
};

int heap_setup(void);
void heap_clean(void);
void* heap_malloc(size_t size);
void* heap_calloc(size_t number, size_t size);
void* heap_realloc(void* memblock, size_t size);
void  heap_free(void* memblock);
enum pointer_type_t get_pointer_type(const void* const pointer);
int heap_validate(void);
size_t   heap_get_largest_used_block_size(void);
int checksum(struct memory_chunk_t* chunk);

#endif //ALOKATOR_HEAP_H
