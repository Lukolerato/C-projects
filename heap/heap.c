#include "heap.h"
#include "custom_unistd.h"
#include <memory.h>

struct memory_manager_t memoryManager;


int heap_setup(void){
    memoryManager.memory_size =0;
    memoryManager.memory_start = custom_sbrk(0);
    if(memoryManager.memory_start == NULL){
        return -1;
    }

    memoryManager.first_memory_chunk = NULL;
    memoryManager.flag = 1;
    return 0;
}
void heap_clean(void){
    custom_sbrk(memoryManager.memory_size * (-1));
    memoryManager.flag = 0;
}
void* heap_malloc(size_t size){
    if(size == 0){
        return NULL;
    }
    if(heap_validate() != 0){
        return NULL;
    }

    struct memory_chunk_t *chunk = memoryManager.first_memory_chunk;

    int sum = size + sizeof(struct memory_chunk_t) + 2*FENCE;

    if(chunk == NULL){ //pierwszy chunk
        chunk = custom_sbrk(sum);
        if(chunk == (void *) -1){
            return NULL;
        }
        chunk->size = size;
        chunk->free = 0;
        chunk->waste = 0;
        memoryManager.memory_size+=sum;
        chunk->next = NULL;
        chunk->prev = NULL;
        memoryManager.first_memory_chunk = chunk;
        for (int i = 0; i < FENCE; ++i){
            *((char *)chunk + sizeof(struct memory_chunk_t) + i) = '#';
        }
        for (int i = 0; i < FENCE; ++i){
            *((char *)chunk + sizeof(struct memory_chunk_t) + FENCE + chunk->size + i) = '#';
        }
        chunk->checksum = checksum(chunk);
        return (char *) chunk + sizeof(struct memory_chunk_t) + FENCE;
    }

    for(;;){
        if(chunk->free == 1 && chunk->size >= size ) { //pirwsze wolne miejsce  dopisac  "&& chunk->next!= NULL"
            chunk->waste = chunk->size - size;
            chunk->size = size;
            chunk->free = 0;
            for (int i = 0; i < FENCE; ++i){
                *((char *)chunk + sizeof(struct memory_chunk_t) + i) = '#'; //dopisane
                *((char *)chunk + sizeof(struct memory_chunk_t) + FENCE + chunk->size + i) = '#';
            }
            chunk->checksum = checksum(chunk);
            return (char *) chunk + sizeof(struct memory_chunk_t) + FENCE;
        }

        struct memory_chunk_t *prev_chunk = chunk;
        chunk = chunk->next;

        if (chunk == NULL) { //ostatni
            chunk = custom_sbrk(sum);
            if(chunk == (void *) -1){
                return NULL;
            }
            chunk->next = NULL;
            chunk->prev = prev_chunk;
            if(prev_chunk != NULL){
                prev_chunk->next = chunk;
                prev_chunk->checksum = checksum(prev_chunk);
            }
            chunk->size = size;
            chunk->free = 0;
            memoryManager.memory_size += sum;
            for (int i = 0; i < FENCE; ++i){
                *((char *)chunk + sizeof(struct memory_chunk_t) + i) = '#';
            }
            for (int i = 0; i < FENCE; ++i){
                *((char *)chunk + sizeof(struct memory_chunk_t) + FENCE + chunk->size + i) = '#';
            }
            chunk->waste = 0;
            chunk->checksum = checksum(chunk);
            return (char *) chunk + sizeof(struct memory_chunk_t) + FENCE;
        }
    }
}
void* heap_calloc(size_t number, size_t size){
    if(size == 0 || number == 0){
        return NULL;
    }

    void *res = heap_malloc(size*number);
    if(res == NULL){
        return NULL;
    }

    for (unsigned int i = 0; i < size*number; ++i) {
        *((char*)res + i) = 0;
    }

    return res;
}
void* heap_realloc(void* memblock, size_t size) {

    if (heap_validate() != 0) {
        return NULL;
    }
    if (size == 0) {
        heap_free(memblock);
        return NULL;
    }
    if (memblock == NULL) {
        void *res = heap_malloc(size);
        return res;
    }

    if (get_pointer_type(memblock) != pointer_valid) {
        return NULL;
    }

    struct memory_chunk_t *chunk = memoryManager.first_memory_chunk;

    if (chunk == NULL) {
        return NULL;
    }
    if (memoryManager.flag == 0) {
        return NULL;
    }

    chunk = (struct memory_chunk_t *) ((char *) memblock - sizeof(struct memory_chunk_t) - FENCE);

    if (size == chunk->size) {
        return memblock;
    }


    if (chunk->size > size) {//mniejszy
        int temp = chunk->size - size;
        chunk->waste += temp;
        chunk->size -= temp;
        for (int i = 0; i < FENCE; ++i) {
            *((char *) chunk + sizeof(struct memory_chunk_t) + FENCE + chunk->size + i) = '#';
        }
        chunk->checksum = checksum(chunk);
        return memblock;

    } else if (chunk->next != NULL && chunk->next->free == 1 &&
               chunk->next->size > size - chunk->size - chunk->waste) {//1
        chunk->next->size -= size - chunk->size - chunk->waste;
        struct memory_chunk_t *nextChunk = chunk->next;
        nextChunk = (struct memory_chunk_t *) ((char *) nextChunk + size - chunk->size - chunk->waste);

        chunk->size = size;
        chunk->waste = 0;
        nextChunk->size = chunk->next->size;
        nextChunk->next = chunk->next->next;
        nextChunk->prev = chunk->next->prev;
        nextChunk->waste = chunk->next->waste;
        nextChunk->free = chunk->next->free;

        if (nextChunk->next != NULL) {
            nextChunk->next->prev = nextChunk;
            nextChunk->next->checksum = checksum(nextChunk->next);
        }

        for (int i = 0; i < FENCE; ++i) {
            *((char *) chunk + sizeof(struct memory_chunk_t) + FENCE + chunk->size + i) = '#';
            *((char *) nextChunk + sizeof(struct memory_chunk_t) + FENCE + i) = '#';
        }
        chunk->next = nextChunk;
        chunk->next->checksum = checksum(chunk->next);
        chunk->checksum = checksum(chunk);
        return memblock;

    } else if (chunk->next != NULL && chunk->next->free == 1 &&
               chunk->next->size + sizeof(struct memory_chunk_t) + 2 * FENCE >
               size - chunk->size - chunk->waste) {//1.ultra fajny

        int temp = chunk->next->size + sizeof(struct memory_chunk_t) + 2 * FENCE + chunk->size + chunk->waste;
        int reszta = temp - size;
        chunk->size = size;
        chunk->waste = reszta;

        chunk->next = chunk->next->next;
        if (chunk->next != NULL) {
            chunk->next->prev = chunk;
            chunk->next->checksum = checksum(chunk->next);
        }

        for (int i = 0; i < FENCE; ++i) {
            *((char *) chunk + sizeof(struct memory_chunk_t) + FENCE + chunk->size + i) = '#';
        }
        chunk->checksum = checksum(chunk);
        return memblock;
    } else if (chunk->size < size) {//2
        if (chunk->next == NULL) {
            int temp = size - chunk->size;
            void *cos = custom_sbrk(temp);

            if (cos == (void *) -1) {
                return NULL;
            }
            memoryManager.memory_size += temp;
            chunk->size = size;
            for (int i = 0; i < FENCE; ++i) {
                *((char *) chunk + sizeof(struct memory_chunk_t) + FENCE + chunk->size + i) = '#';
            }
            chunk->checksum = checksum(chunk);
            return memblock;
        } else {
            //4
            char *res = heap_malloc(size);
            if (!res) {
                return NULL;
            }

            memcpy(res, (char *) chunk + sizeof(struct memory_chunk_t) + FENCE, chunk->size);
            heap_free((char *) chunk + sizeof(struct memory_chunk_t) + FENCE);

            return res;

        }
    }

    if (chunk == NULL) {
        return NULL;
    }

    return (char *) chunk + sizeof(struct memory_chunk_t) + FENCE;
}
void  heap_free(void* memblock){
    if(memblock == NULL || memoryManager.memory_start == NULL){
        return;
    }
    if(memblock < memoryManager.memory_start){
        return;
    }
    if(get_pointer_type(memblock) != pointer_valid){
        return;
    }
    struct memory_chunk_t *chunk;

    chunk = (struct memory_chunk_t*)((char*)memblock -(sizeof(struct memory_chunk_t) + FENCE));

    if(chunk->free == 1){
        return;
    }

    if((chunk == memoryManager.first_memory_chunk) && (chunk->next == NULL)){
        custom_sbrk(memoryManager.memory_size * (-1));
        memoryManager.memory_size = 0;
        memoryManager.first_memory_chunk = NULL;
        return;
    }

    if(chunk->next == NULL){
        chunk->prev->next = NULL;
        chunk->prev->checksum = checksum(chunk->prev);
        custom_sbrk(chunk->size * (-1));
        memoryManager.memory_size -= chunk->size;
        return;
    }

    chunk->free = 1;
    chunk->size += chunk->waste;
    chunk->waste = 0;


    if(chunk->next->free == 1){
        chunk->size += chunk->next->size + sizeof(struct memory_chunk_t) + 2*FENCE;
        chunk->next =  chunk->next->next;
        if(chunk->next != NULL){
            chunk->next->prev = chunk;
            chunk->next->checksum = checksum(chunk->next);
        }
    }

    if(chunk->prev == NULL){
        return;
    }
    if(chunk->prev->free == 1){
        chunk->prev->size += chunk->size + sizeof(struct memory_chunk_t) + 2*FENCE;
        chunk->prev->next = chunk->next;
        if(chunk->next != NULL){
            chunk->next->prev = chunk->prev;
            chunk->next->checksum = checksum(chunk->next);
        }
    }

}

enum pointer_type_t get_pointer_type(const void* const pointer){
    if(pointer == NULL){
        return pointer_null;
    }

    struct memory_chunk_t* chunk = memoryManager.first_memory_chunk;
    if(chunk == NULL){
        return pointer_unallocated;
    }
    for(;;){
        if(pointer >= (void*)chunk && pointer < (void*)((char*)chunk + sizeof (struct memory_chunk_t) + 2*FENCE + chunk->size + chunk->waste)){

            if(chunk->free == 1){
                return pointer_unallocated;
            }
            if(pointer<(void*)((char*)chunk + sizeof (struct memory_chunk_t))){
                return pointer_control_block;
            } else if(pointer<(void*)((char*)chunk + sizeof (struct memory_chunk_t) + FENCE)){
                return pointer_inside_fences;
            } else if(pointer == (void*)((char*)chunk + sizeof (struct memory_chunk_t) + FENCE)){
                return pointer_valid;
            } else if(pointer < (void*)((char*)chunk + sizeof (struct memory_chunk_t) + FENCE + chunk->size)){
                return pointer_inside_data_block;
            }else if(pointer < (void*)((char*)chunk + sizeof (struct memory_chunk_t) + 2*FENCE + chunk->size)) {
                return pointer_inside_fences;
            }
            else{
                return pointer_unallocated;
            }
        }
        chunk = chunk->next;
        if(chunk == NULL){
            break;
        }
    }
    return pointer_unallocated;

}
int heap_validate(void){
    struct memory_chunk_t* chunk = memoryManager.first_memory_chunk;

    if(chunk == NULL && memoryManager.memory_start == NULL || memoryManager.flag == 0){
        return 2;
    }

    if(chunk == NULL){
        return 0;
    }
    int k = 0;
    for (;;) {
        if(chunk->free == 1){
            return 0;
        }

        if(chunk->checksum != checksum(chunk)){
            return 3;
        }

        char* check_fence = (char *) chunk + sizeof(struct memory_chunk_t);
        for (int i = 0; i < FENCE; ++i) {
            if(*check_fence != '#'){
                return 1;
            }
            check_fence++;
        }
        check_fence+=chunk->size;
        for (int i = 0; i < FENCE; ++i) {
            if (*check_fence != '#') {
                return 1;
            }
            check_fence++;
        }
        chunk = chunk->next;
        k++;

        if(chunk == NULL){
            break;
        }

    }

    return 0;

}
size_t   heap_get_largest_used_block_size(void){
    struct memory_chunk_t* chunk = memoryManager.first_memory_chunk;

    if(memoryManager.memory_start == NULL || heap_validate() || chunk == NULL){
        return 0;
    }

    size_t max_size = 0;
    for (;;) {

        if(chunk->size > max_size && chunk->free == 0){
           max_size = chunk->size;
        }

        chunk= chunk->next;
        if(chunk == NULL){
            break;
        }
    }
    return max_size;
}

int checksum(struct memory_chunk_t* chunk){

    int sum = 0;
    uint8_t *ptr = (uint8_t *) &chunk->size;

    for (size_t i = 0; i < sizeof(chunk->size); ++i) {
        sum += ptr[i];
    }
    ptr = (uint8_t *) &chunk->waste;
    for (size_t i = 0; i < sizeof(chunk->waste); ++i) {
        sum += ptr[i];
    }
    ptr = (uint8_t *) &chunk->free;
    for (size_t i = 0; i < sizeof(chunk->free); ++i) {
        sum += ptr[i];
    }

    if(chunk->next!=NULL){
        ptr = (uint8_t *) &chunk->next;
        for (size_t i = 0; i < sizeof(struct memory_chunk_t*); ++i) {
            sum += ptr[i];
        }
    }
    if(chunk->prev != NULL){
        ptr = (uint8_t *) &chunk->prev;
        for (size_t i = 0; i < sizeof(struct memory_chunk_t*); ++i) {
            sum += ptr[i];
        }
    }


    return sum;
}

