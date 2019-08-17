#include <stdint.h>
#include <stdlib.h>
#include <string.h>

extern void debug(const char *fmt, ...);
extern void *sbrk(intptr_t increment);
void check_freelist();

unsigned int max_size;
size_t *freelist;
size_t dataoffset = sizeof(size_t *) + 2 * sizeof(size_t); //(block size)8byte + (data size)data8byte + (next)8byte

void *myalloc(size_t size)
{
    //debug("malloc\n");
    void *p;
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
        free_ptr = (size_t *)free_ptr[2];
    }

    if (found != NULL){ //found matching space
        
        if (before == free_ptr){ //delete at front
            freelist = (size_t *)found[2];
        }
        else { //delete at somewhere
            before[2] = (size_t *)found[2];
        }
        found[1] = size;
        p = &found[3];
        //debug("[+] alloc_list(%u/%u): base(%p), data(%p)\n", found[1], found[0], found, p);
        //debug("    max: %u\n", max_size);
        //check_freelist();
        return p;
    }
    else { //no freelist space
        p = sbrk(new_size);
        size_t *new_p = p;
        new_p[0] = size; //block size
        new_p[1] = size; //data size
        new_p[2] = NULL; //next
        p = &new_p[3];
        max_size += new_size;
        //debug("[+] alloc_new(%u): base(%p), data(%p)\n", (unsigned int)size, new_p, p);
        //debug("    max: %u\n", max_size);
        //check_freelist();
        return p;
    }
}

void *myrealloc(void *ptr, size_t size)
{
    //debug("re");
    void *p; //return value
    size_t new_size = size + dataoffset;
    //search freelist
    size_t *new_ptr = ptr;
    //if (ptr) debug("realloc ptr(%p, %u/%u) size(%u)\n", ptr, *(new_ptr - 2), *(new_ptr - 3), size); //new_ptr-2 : data size
    
    size_t *free_ptr = freelist;
    size_t *before = free_ptr;
    size_t *found = NULL;
    while (free_ptr != NULL){ //find matching free space
        if (free_ptr[0] >= size){
            found = free_ptr;
            break;
        }
        before = free_ptr;
        free_ptr = (size_t *)free_ptr[1];
    }

    if (found != NULL){ //found matching space
        
        if (before == free_ptr){ //delete at front
            freelist = (size_t *)found[2];
        }
        else { //delete at somewhere
            before[2] = (size_t *)found[2];
        }
        p = &found[3];
        if (ptr){ //ptr = NULL -> same as malloc
            int sss = *(new_ptr - 2); //ptr's original data size
            sss = sss > size ? size : sss; //if data bigger than size
            memcpy(p, ptr, sss); //just copy reduced size
        }
        //debug("[+] realloc_list(%u): base(%p), data(%p)\n", (unsigned int)size, found, p);
        //debug("    max: %u\n", max_size);
        //check_freelist();
        return p;
    }
    else {//no freelist space
        p = NULL;
        if (size != 0)
        {
            p = sbrk(new_size);
            size_t *new_p = p;
            new_p[0] = size;
            new_p[1] = size;
            new_p[2] = NULL;
            p = &new_p[3];
            if (ptr){ //ptr = NULL -> same as malloc
                int sss = *(new_ptr - 2); //ptr's original data size
                sss = sss > size ? size : sss; //if data bigger than size
                memcpy(p, ptr, sss); //just copy reduced size
            }
            max_size += new_size;
            //debug("    max: %u\n", max_size);
        }
        //debug("[+] realloc_new(%p, %u): data(%p)\n", ptr, (unsigned int)size, p);
        //check_freelist();
        return p;
    }
}

void myfree(void *ptr) //ptr: pointing `data`
{
    //debug("[+] free(%p)\n", ptr);
    if (ptr == NULL) return;
    size_t *tmp = ptr; 
    tmp = tmp - 3; // point block size

    //

    size_t *free_ptr = freelist;
    size_t *before = free_ptr;
    tmp[1] = 0; //data size -> 0
    if (free_ptr == NULL){ //when list is null
        tmp[2] = NULL;
        freelist = tmp;
    }
    else {
        while(free_ptr != NULL){
            if (free_ptr[0] >= tmp[0]) break; //compare block size
            //make ascending ordered list
            before = free_ptr;
            free_ptr = (size_t *)free_ptr[2];
        }
        
        //insert at right place
        tmp[2] = free_ptr;
        if (before == free_ptr){ //insert at front
            freelist = tmp;
        }
        else { //insert at somewhere
            before[2] = tmp;
        }
    }

    //check_freelist();
}

void check_freelist(){
    size_t *tmp = freelist;
    int i = 1;
    while (tmp != NULL){
        debug("    list %d: addr(%p), block size(%u), data size(%u), next(%p)\n", i, tmp, tmp[0], tmp[1], tmp[2]);
        tmp = tmp[2];
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