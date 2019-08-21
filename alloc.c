/* Implemented by Segregated List */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

extern void debug(const char *fmt, ...);
extern void *sbrk(intptr_t increment);
void check_seglist();
void insert_seglist(size_t *tmp);
void coalesce(size_t *ptr);
void delete_seglist(size_t *tmp);
int search_match_list(size_t size);

unsigned int max_size;
size_t *seglist[35];
size_t seglist_num = 35;
size_t seglist_size[34] = {
    8, 16, 24, 32, 40, 48, 64, 72, 96, 128, 192, 256, 384, 512, 768, 1024, 1536, 2048, 
    3072, 4096, 6144, 8192, 16384, 32768, 65536, 131072, 262144, 524288, 1048576,
    2097152, 4194354, 8388608, 16777216, 33554432
}; //list 0~seglist_num max size declared, list [seglist_num] not declared
/*
TOTAL SIZE = BLOCKSIZE + 32
*/

size_t hdrsize = sizeof(size_t *) + 2 * sizeof(size_t); 
size_t prevsize = sizeof(size_t);
size_t heapbase = 0;
size_t heapend = 0;
//hdr = (block size)8byte + (data size)8byte + (next)8byte
//footer = (prev block size)8byte

void *myalloc(size_t size)
{
    //debug("malloc\n");
    void *p;
    size_t new_size = size + hdrsize + prevsize;
    //search seglist
    int n;// = search_match_list(size);
    n = 0;
    if (size > seglist_size[seglist_num - 2]){
        n = seglist_num - 1;
    }
    else {
        for (; n < seglist_num; n++){
            if (size <= seglist_size[n]){
                break;
            }
        }
    }
    //debug("find size %d-%d\n",seglist_size[n-1], seglist_size[n]);
    size_t *free_ptr = seglist[n];
    size_t *before = free_ptr;
    size_t *found = NULL;
    int f_split = 0;
    while (free_ptr != NULL){ //first fit from ascending order
        if (free_ptr[0] >= size){ //compare block size
            found = free_ptr;
            if (free_ptr[0] >= size + hdrsize + prevsize + seglist_size[0]) f_split = 1; //size difference >= smallest size + 32(header)
            break;
        }
        before = free_ptr;
        free_ptr = (size_t *)free_ptr[2];
    }
    //not found -> search bigger seglist
    //debug("n(%d), found(%p)\n", n, found);
    while ((found == NULL) && (++n < seglist_num - 1)){
        free_ptr = seglist[n];
        before = free_ptr;
        while (free_ptr != NULL){ //first fit from ascending order
            if (free_ptr[0] >= size){ //compare block size
                found = free_ptr;
                if (free_ptr[0] >= size + hdrsize + prevsize + seglist_size[0]) f_split = 1; //size difference >= smallest size + 32(header)
                break;
            }
            before = free_ptr;
            free_ptr = (size_t *)free_ptr[2];
        }
    }
    if (found != NULL){ //found matching space
        if (before == free_ptr){ //delete at front
            seglist[n] = (size_t *)found[2];
        }
        else { //delete at somewhere
            before[2] = (size_t *)found[2];
        }
        found[1] = size;
        p = &found[3];

        if (f_split == 1){
            //split space
            size_t *remaining = (unsigned char *)found + new_size;
            remaining[0] = found[0] - new_size;
            *(size_t *)((unsigned char *)remaining + remaining[0] + hdrsize) = remaining[0];
            //debug("remain(%p), found(%p), remainingsize(%u)\n",remaining, found, remaining[0]);
            //insert remaining to seglist

            found[0] = size;
            *(size_t *)((unsigned char *)found + found[0] + hdrsize) = found[0];

            insert_seglist(remaining);

            coalesce(remaining);
        }
        //debug("[+] alloc_list(%u/%u): base(%p), data(%p)\n", found[1], found[0], found, p);
    }
    else { //no seglist space
        p = sbrk(new_size);
        size_t *new_p = p;
        new_p[0] = size; //block size
        new_p[1] = size; //data size
        new_p[2] = NULL; //next
        *(size_t *)((unsigned char *)new_p + new_p[0] + hdrsize) = new_p[0];
        //debug("%u, %u", new_p[0], *(size_t *)((unsigned char *)new_p + new_p[0] + hdrsize));
        if (heapbase == 0){
            heapbase = p;
            heapend = p;
        } 
        p = &new_p[3];
        max_size += new_size;
        heapend += new_size;
        //debug("[+] alloc_new(%u): base(%p), data(%p)\n", (unsigned int)size, new_p, p);
    }
    //debug("    max: %u\n", max_size);
    //check_seglist();
    return p;
}

void *myrealloc(void *ptr, size_t size)
{
    //debug("realloc(%p, %u)\n", ptr, size);
    if (ptr == NULL && size == 0)  //realloc(NULL, 0);
        return 0;
    if (ptr == NULL && size != 0)  //realloc(NULL, size);
        return myalloc(size);

    void *p; //return value
    size_t new_size = size + hdrsize + prevsize;
    //search seglist
    size_t *new_ptr = (size_t *)ptr - 3;
    //if (ptr) debug("[+] realloc ptr(%p, %u/%u) size(%u)\n", ptr, new_ptr[1], new_ptr[0], size); //new_ptr-2 : data size
    
    if (ptr != NULL && size == 0){ //realloc(ptr, 0)
        //debug("   [-] realloc(ptr, 0) -> ");
        myfree(ptr);
        return 0;   
    }
    
    if ((new_ptr[0] > size) && (new_ptr[0] - size < hdrsize + prevsize + seglist_size[0])){ 
        //smaller , block size difference < 40: just use same space
        new_ptr[1] = size;
        return &new_ptr[3];
    }
    else if((new_ptr[0] > size) && (new_ptr[0] - size >= hdrsize + prevsize + seglist_size[0])){ 
        //smaller , block size difference >= 40: split space
        size_t *remaining = (unsigned char *)new_ptr + new_size;
        remaining[0] = new_ptr[0] - new_size;
        *(size_t *)((unsigned char *)remaining + remaining[0] + hdrsize) = remaining[0];
        //debug("remain(%p), size(%u/%u), next(%p)\n", remaining, remaining[1], remaining[0], remaining[2]);
        //debug("new_ptr(%p), size(%u/%u), next(%p)\n", new_ptr, new_ptr[1], new_ptr[0], new_ptr[2]);

        new_ptr[0] = size;
        new_ptr[1] = size;
        *(size_t *)((unsigned char *)new_ptr + new_ptr[0] + hdrsize) = new_ptr[0];

        //insert remaining to seglist
        insert_seglist(remaining);
        
        coalesce(remaining);

        return &new_ptr[3];
    }
    else { //bigger: free and get new space
        //debug("   [-] realloc_new -> ");
        myfree(ptr);
        //debug("   [-] realloc_new -> ");
        return myalloc(size);
    }
}

void myfree(void *ptr) //ptr: pointing `data`
{
    //debug("[+] free(%p)\n", ptr);
    if (ptr == NULL) return;
    size_t *tmp = ptr; 
    tmp = tmp - 3; // point [0]:block size
    //debug("    base(%p), data(%u/%u)\n", tmp, tmp[1], tmp[0]);
    insert_seglist(tmp);

    coalesce(tmp);

    //check_seglist();
}

void check_seglist(){
    return;
    size_t *tmp;
    for (int n = 0; n < seglist_num; n++){
        int i = 1;
        tmp = seglist[n];
        if (tmp == NULL) continue;
        if (n < seglist_num - 1) debug("    SEGLIST %d(~%d):\n", n, seglist_size[n]);
        else debug("    SEGLIST %d(%d~):\n", n, seglist_size[n-1]);
        while (tmp != NULL){
            debug("      ->list %d: addr(%p), block size(%u), next(%p), block size2(%u)\n", i, tmp, tmp[0], tmp[2], *(size_t *)((unsigned char *)tmp + tmp[0] + hdrsize));
            tmp = tmp[2];
            i++;
        }
    }
}

void insert_seglist(size_t *tmp){ //tmp pointing the header
    //find matching seglist
    int n = 0;
    //n = search_match_list(tmp[0]);
    if (tmp[0] > seglist_size[seglist_num - 2]){
        n = seglist_num - 1;
    }
    else {
        for (; n < seglist_num; n++){
            if (tmp[0] <= seglist_size[n]){
                break;
            }
        }
    }
    //debug("insert ptr(%p), size(%u), n(%d)", tmp, tmp[0], n);
    size_t *free_ptr = seglist[n];
    size_t *before = free_ptr;
    tmp[1] = 0; //data size -> 0
    if (free_ptr == NULL){ //when list is null
        tmp[2] = NULL;
        seglist[n] = tmp;
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
            seglist[n] = tmp;
        }
        else { //insert at somewhere
            before[2] = tmp;
        }
    }
}

void coalesce(size_t *ptr){ //ptr pointing the header
    //debug("coalesce start\n");
    //debug("my size(%u)", ptr[0]);

    size_t *next_addr = (unsigned char *)ptr + ptr[0] + hdrsize + prevsize;;
    //debug("ptr(%p) heapbase(%p) next(%p) heapend(%p)",ptr, heapbase, next_addr, heapend);
COALBOTH:
    if ((ptr != heapbase) && (next_addr != heapend)){
        size_t sizetmp = *(ptr - 1);
        //debug("prev size(%u)\n", sizetmp);
        size_t *prev_addr = (unsigned char *)ptr - sizetmp - hdrsize - prevsize;
        //debug("%p", prev_addr);
        //debug("prev blocksize(%u) datasize(%u)", prev_addr[0], prev_addr[1]);
        //debug("prev addr(%p)\n", prev_addr);
        //debug("next blocksize(%u) datasize(%u)", next_addr[0], next_addr[1]);
        //debug("next addr(%p)\n", next_addr);

        //if prev = free && next = free
        if (next_addr[1] == 0 && prev_addr[1] == 0){
            delete_seglist(prev_addr);
            delete_seglist(ptr);
            delete_seglist(next_addr);
            
            //insert new one
            prev_addr[0] = 2 * (hdrsize + prevsize) + prev_addr[0] + ptr[0] + next_addr[0];
            prev_addr[1] = 0;
            *(size_t *)((unsigned char *)prev_addr + prev_addr[0] + hdrsize) = prev_addr[0];
            insert_seglist(prev_addr);
        }
        else if (next_addr[1] == 0){
            delete_seglist(ptr);
            delete_seglist(next_addr);
            
            //insert new one
            ptr[0] = hdrsize + prevsize + ptr[0] + next_addr[0];
            ptr[1] = 0;
            *(size_t *)((unsigned char *)ptr + ptr[0] + hdrsize) = ptr[0];
            insert_seglist(ptr);
        }
        else if (prev_addr[1] == 0){
            delete_seglist(prev_addr);
            delete_seglist(ptr);
            
            //insert new one
            prev_addr[0] = hdrsize + prevsize + prev_addr[0] + ptr[0];
            prev_addr[1] = 0;
            *(size_t *)((unsigned char *)prev_addr + prev_addr[0] + hdrsize) = prev_addr[0];
            insert_seglist(prev_addr);
        }
        return;
    }

COALNEXT:
    if ((ptr == heapbase) && (next_addr != heapend)){
        //debug("next blocksize(%u) datasize(%u)", next_addr[0], next_addr[1]);
        //debug("next addr(%p)\n", next_addr);
        //if prev != free && next = free;
        if (next_addr[1] == 0){
            delete_seglist(ptr);
            delete_seglist(next_addr);
            
            //insert new one
            ptr[0] = hdrsize + prevsize + ptr[0] + next_addr[0];
            ptr[1] = 0;
            *(size_t *)((unsigned char *)ptr + ptr[0] + hdrsize) = ptr[0];
            insert_seglist(ptr);
        }
        return;
    }

COALPREV:
    if ((ptr != heapbase) && (next_addr == heapend)){
        size_t sizetmp = *(ptr - 1);
        //debug("prev size(%u)\n", sizetmp);
        size_t *prev_addr = (unsigned char *)ptr - sizetmp - hdrsize - prevsize;
        //debug("prev blocksize(%u) datasize(%u)", prev_addr[0], prev_addr[1]);
        //debug("prev addr(%p)\n", prev_addr);
        //if prev = free && next != free
        if (prev_addr[1] == 0){
            delete_seglist(prev_addr);
            delete_seglist(ptr);
            
            //insert new one
            prev_addr[0] = hdrsize + prevsize + prev_addr[0] + ptr[0];
            prev_addr[1] = 0;
            *(size_t *)((unsigned char *)prev_addr + prev_addr[0] + hdrsize) = prev_addr[0];
            insert_seglist(prev_addr);
        }
        return;
    }
}

void delete_seglist(size_t *tmp){ //tmp pointing the header
    //find matching seglist
    //debug("deleting addr(%p) size(%u/%u)\n", tmp, tmp[1], tmp[0]);
    int n = 0;
    if (tmp[0] > seglist_size[seglist_num - 2]){
        n = seglist_num - 1;
    }
    else {
        for (; n < seglist_num; n++){
            if (tmp[0] <= seglist_size[n]){
                break;
            }
        }
    }
    //debug("found n(%d)\n",n);
    size_t *free_ptr = seglist[n];
    size_t *before = free_ptr;
    while(free_ptr != tmp){ //find tmp
        before = free_ptr;
        free_ptr = (size_t *)free_ptr[2];
    }

    if (before == free_ptr){ //delete at front
        seglist[n] = (size_t *)free_ptr[2];
    }
    else { //delete at somewhere
        before[2] = (size_t *)free_ptr[2];
    }
}

/*

seglist
*************************
    block size(size_t)      [0]
*************************
    data size(size_t)       [1]
*************************
      next(size_t *)        [2]
*************************
        data/block          [3] <- return value
*************************
    block size(size_t)      [block_size + dataoffset] <- for coaslescing
#########################

*/