#include <stdint.h>
#include <stdlib.h>
#include <string.h>

extern void debug(const char *fmt, ...);
extern void *sbrk(intptr_t increment);
void check_freelist();

unsigned int max_size;
size_t *freelist;
size_t dataoffset = sizeof(size_t *) + sizeof(size_t); //8byte + 8byte

void *myalloc(size_t size)
{
    debug("2\n");
    size_t new_size = size + dataoffset;
    //search freelist
    size_t *free_ptr = freelist;
    size_t *before = free_ptr;
    size_t *found = NULL;
    while (free_ptr != NULL){ //first fit from ascending order
        if (free_ptr[0] >= size){
            found = free_ptr;
            break;
        }
        before = free_ptr;
        free_ptr = (size_t *)free_ptr[1];
    }

    if (found != NULL){ //found matching space
        
        if (before == free_ptr){ //delete at front
            freelist = (size_t *)found[1];
        }
        else { //delete at somewhere
            before[1] = (size_t *)found[1];
        }
        void *p = &found[2];
        debug("[+] alloc_list(%u): base(%p), data(%p)\n", (unsigned int)size, found, p);
        debug("    max: %u\n", max_size);
        check_freelist();
        return p;
    }
    else { //no freelist space
        void *p = sbrk(new_size);
        size_t *new_p = p;
        new_p[0] = size;
        new_p[1] = NULL;
        p = &new_p[2];
        debug("[+] alloc_new(%u): base(%p), data(%p)\n", (unsigned int)size, new_p, p);
        max_size += new_size;
        debug("    max: %u\n", max_size);
        check_freelist();
        return p;
    }
}

void *myrealloc(void *ptr, size_t size)
{
    debug("3\n");
    //debug("1\n");
    size_t new_size = size + dataoffset;
    //search freelist
    size_t *new_ptr = ptr;
    size_t *free_ptr = freelist;
    size_t *before = free_ptr;
    size_t *found = NULL;
    while (free_ptr != NULL){
        if (free_ptr[0] >= size){
            found = free_ptr;
            break;
        }
        before = free_ptr;
        free_ptr = (size_t *)free_ptr[1];
    }

    if (found != NULL){ //found matching space
        
        if (before == free_ptr){ //delete at front
            freelist = (size_t *)found[1];
        }
        else { //delete at somewhere
            before[1] = (size_t *)found[1];
        }
        void *p = &found[2];
        if (ptr) //ptr = NULL -> same as malloc
            memcpy(p, ptr, *(new_ptr - 2));
        debug("[+] realloc_list(%u): base(%p), data(%p)\n", (unsigned int)size, found, p);
        debug("    max: %u\n", max_size);
        check_freelist();
        return p;
    }
    else {//no freelist space
        void *p = NULL;
        if (size != 0)
        {
            p = sbrk(new_size);
            size_t *new_p = p;
            new_p[0] = size;
            new_p[1] = NULL;
            p = &new_p[2];
            if (ptr) //ptr = NULL -> same as malloc
                memcpy(p, ptr, *(new_ptr - 2));
            max_size += new_size;
            debug("    max: %u\n", max_size);
        }
        debug("[+] realloc_new(%p, %u): data(%p)\n", ptr, (unsigned int)size, p);
        check_freelist();
        return p;
    }
}

void myfree(void *ptr) //ptr: pointing `data`
{
    debug("[+] free(%p)\n", ptr);
    if (ptr == NULL) return;
    size_t *tmp = ptr; 
    tmp = tmp - 2; //tmp[-2]
    //debug("    size:%u(%p), next(%p), data(%p)\n", tmp[0], tmp, tmp[1], tmp + 2);

    size_t *free_ptr = freelist;
    size_t *before = free_ptr;
    //debug("1\n");
    if (free_ptr == NULL){ //attach at end of list
        tmp[1] = NULL;
        freelist = tmp;
    }
    else {
        while(free_ptr != NULL){
            if (free_ptr[0] >= tmp[0]) break;
            //make ascending ordered list
            //debug("    freeptr(%p) freeptr[0](%u) freeptr[1](%p)\n", free_ptr, free_ptr[0], free_ptr[1]);
            before = free_ptr;
            free_ptr = (size_t *)free_ptr[1];
        }
        //debug("    freeptr(%p) freeptr[0](%u) freeptr[1](%p) tmp[0](%u)\n", free_ptr, free_ptr[0], free_ptr[1], tmp[0]);
        //debug("    before[1](%p) before(%p)\n", before[1], before);
        
        //attach at right place
        tmp[1] = free_ptr;
        if (before == free_ptr){ //insert at front
            freelist = tmp;
        }
        else { //insert at somewhere
            before[1] = tmp;
        }
    }

    //debug("3\n");
    check_freelist();
}

void check_freelist(){
    size_t *tmp = freelist;
    int i = 1;
    while (tmp != NULL){
        debug("    list %d: addr(%p), size(%u), next(%p)\n", i, tmp, tmp[0], tmp[1]);
        tmp = tmp[1];
        i++;
    }
}

/*

freelist
*************************
   size(unsigned long)
*************************
     *next(uint64_t)
*************************
          data
*************************

*/