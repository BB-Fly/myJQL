#include "buffer_pool.h"
#include "file_io.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void init_buffer_pool(const char *filename, BufferPool *pool) {
    if(open_file(&pool->file,filename)==FILE_IO_SUCCESS){
        for(int i=0;i<CACHE_PAGE;i++){
            pool->addrs[i] =-1;
            pool->cnt[i] = 32;
            pool->ref[i] = 0;
        }
    }else{
        printf("init_buffer_pool failed! Open file failed.\n");
        //my_test_print("init_buffer_pool failed! Open file failed.\n");
    }
    return;
}

void close_buffer_pool(BufferPool *pool) {
    for(int i=0;i<CACHE_PAGE;i++){
        write_page(&pool->pages[i],&pool->file,pool->addrs[i]);
    }
    if(close_file(&pool->file)==FILE_IO_SUCCESS){
        return;
    }
    printf("close_buffer_pool failed! Close file failed.");
    my_test_print("close_buffer_pool failed! close file failed.\n");
    return;
}

Page *get_page(BufferPool *pool, off_t addr) {
    // my_test_print("\tget page: ");
    // my_test_p_num(addr);
    if(addr > pool->file.length){
        printf("get_page failed! addr over.\n");
        my_test_print("get_page failed! addr over.\n");
        return NULL;
    }
    short idx = -1, ans = -1;
    size_t cnt = INT_MAX;

    for(short i=0;i<CACHE_PAGE;i++){
        if(pool->addrs[i] == addr){
            pool->cnt[i] += 2;
            pool->cnt[i] = pool->cnt[i] < 128 ? pool->cnt[i]:128;
            pool->ref[i] = 1;
            ans = i;
        }else if(pool->addrs[i] == -1){
            idx = i;
            break;
        }
        else{
            if(pool->ref[i]==0){
                if(pool->cnt[i]!=0){
                    pool->cnt[i]--;
                }
                if(pool->cnt[i]<cnt){
                    idx = i;
                    cnt = pool->cnt[i];
                }
            }
        }
    }

    if(ans != -1){
        return &pool->pages[ans];
    }
    if(idx==-1){
        printf("get page failed! room over!\n");
        my_test_print("get_page failed! room over.\n");
        return NULL;
    }

    if(write_page(&pool->pages[idx],&pool->file,pool->addrs[idx])!=FILE_IO_SUCCESS){
        printf("get page failed! write page fail\n");
    }

    if(addr==pool->file.length){
        if(write_page(&pool->pages[idx],&pool->file,addr)==FILE_IO_SUCCESS){
            pool->addrs[idx] = addr;
            pool->cnt[idx] = 32;
            pool->ref[idx] = 1;
            return &pool->pages[idx];
        }
        printf("get page failed! file io failed!\n");
        my_test_print("get_page failed! file io failed.\n");
        return NULL;
    }

    if(read_page(&pool->pages[idx],&pool->file,addr)==FILE_IO_SUCCESS){
        pool->addrs[idx] = addr;
        pool->cnt[idx] = 32;
        pool->ref[idx] = 1;
        return &pool->pages[idx];
    }
    printf("get_page failed! Read page failed.");
    my_test_print("get_page failed! read page failed.\n");
    return NULL;
}

void release(BufferPool *pool, off_t addr) {
    // my_test_print("\trelease page: ");
    // my_test_p_num(addr);
    for(int i=0;i<CACHE_PAGE;i++){
        if(pool->addrs[i]==addr){
            pool->ref[i] = 0;
            return;
        }
    }
    printf("release failed! invalid addr.\n");
    my_test_print("release failed! invalid addr.\n");
    return;
}

/* void print_buffer_pool(BufferPool *pool) {
} */

/* void validate_buffer_pool(BufferPool *pool) {
} */
