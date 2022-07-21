#include "hash_map.h"

#include <stdio.h>
#include <string.h>

void hash_table_init(const char *filename, BufferPool *pool, off_t n_directory_blocks) {
    init_buffer_pool(filename, pool);
    /* TODO: add code here */
    if(pool->file.length==0){
        HashMapControlBlock *ctrl = (HashMapControlBlock*)get_page(pool, 0);
        ctrl->max_size = n_directory_blocks * HASH_MAP_DIR_BLOCK_SIZE;
        ctrl->n_directory_blocks = n_directory_blocks;
        ctrl->free_block_head = (ctrl->n_directory_blocks+1) * PAGE_SIZE;
        ctrl->last_addr = ctrl->free_block_head + 128 * PAGE_SIZE;

        HashMapDirectoryBlock *dir_block;
        for(short i=1;i<=n_directory_blocks;i++){
            dir_block = (HashMapDirectoryBlock*)get_page(pool, i*PAGE_SIZE);
            memset(dir_block, 0, sizeof(HashMapDirectoryBlock));
            release(pool,i*PAGE_SIZE);
        }

        HashMapBlock* block;
        off_t block_addr = ctrl->free_block_head, next_addr = block_addr+PAGE_SIZE;
        while(block_addr<ctrl->last_addr){
            block = (HashMapBlock*)get_page(pool, block_addr);
            memset(block, 0, sizeof(HashMapBlock));
            block->n_items = 0;
            block->next = next_addr;
            release(pool, block_addr);
            block_addr = next_addr;
            next_addr += PAGE_SIZE;
        }
        release(pool, 0);
        printf("Hash table initialized !\n");
        //print_hash_table(pool);
    }
    return;
}

void hash_table_close(BufferPool *pool) {
    close_buffer_pool(pool);
    printf("hash table close!\n");
}

void hash_table_insert(BufferPool *pool, short size, off_t addr) {
    //print_hash_table(pool);
    //printf("now in hash insert!, target is size(%lld) = %d\n",addr, size);
    //my_test_print("now in hash insert\n");

    HashMapControlBlock *ctrl = (HashMapControlBlock*)get_page(pool, 0);
    if(size<0||size>=ctrl->max_size){
        printf("hash_insert failed! inavalid size: %d.\n",size);
        release(pool, 0);
        return;
    }

    HashMapDirectoryBlock *dir_block =
        (HashMapDirectoryBlock*)get_page(pool, (size / HASH_MAP_DIR_BLOCK_SIZE + 1) * PAGE_SIZE);

    off_t block_addr = dir_block->directory[size % HASH_MAP_DIR_BLOCK_SIZE];
    HashMapBlock *block = NULL;
    HashMapBlock *tmp_block = NULL;
    off_t next_addr = 0;

    if(block_addr==0){
        block_addr = ctrl->free_block_head;
        if(ctrl->free_block_head==ctrl->last_addr){
            tmp_block = (HashMapBlock *)get_page(pool,ctrl->last_addr);
            dir_block->directory[size % HASH_MAP_DIR_BLOCK_SIZE] = ctrl->last_addr;

            ctrl->last_addr += PAGE_SIZE;
            ctrl->free_block_head += PAGE_SIZE;

            memset(tmp_block, 0, sizeof(HashMapBlock));
            tmp_block->n_items = 1;
            tmp_block->next = 0;
            tmp_block->table[0] = addr;
            release(pool, ctrl->last_addr - PAGE_SIZE);
            
            release(pool,(size / HASH_MAP_DIR_BLOCK_SIZE + 1) * PAGE_SIZE);
            release(pool,0);
            return;
        }
        block = (HashMapBlock*)get_page(pool,block_addr);
        block->table[block->n_items++] = addr;
        next_addr = block->next;
        block->next = 0;
        dir_block->directory[size % HASH_MAP_DIR_BLOCK_SIZE] = block_addr;
        ctrl->free_block_head = next_addr;
        release(pool, block_addr);
        release(pool, (size / HASH_MAP_DIR_BLOCK_SIZE + 1) * PAGE_SIZE);
        release(pool, 0);
        return;
    }

    block = (HashMapBlock*)get_page(pool, block_addr);
    next_addr = block->next;
    while(next_addr!=0){
        release(pool,block_addr);
        block_addr = next_addr;
        block = (HashMapBlock*)get_page(pool, block_addr);
        next_addr = block->next;
    }

    if(block->n_items<HASH_MAP_BLOCK_SIZE){
        block->table[block->n_items++] = addr;
        release(pool,block_addr);
        release(pool,(size / HASH_MAP_DIR_BLOCK_SIZE + 1) * PAGE_SIZE);
        release(pool,0);
        return;
    }
    if(ctrl->free_block_head==ctrl->last_addr){
        tmp_block = (HashMapBlock *)get_page(pool,ctrl->last_addr);
        block->next = ctrl->last_addr;

        ctrl->last_addr += PAGE_SIZE;
        ctrl->free_block_head += PAGE_SIZE;

        memset(tmp_block, 0, sizeof(HashMapBlock));
        tmp_block->n_items = 1;
        tmp_block->next = 0;
        tmp_block->table[0] = addr;
        release(pool, ctrl->last_addr - PAGE_SIZE);
        
        release(pool,block_addr);
        release(pool,(size / HASH_MAP_DIR_BLOCK_SIZE + 1) * PAGE_SIZE);
        release(pool,0);
        return;
    }
    next_addr = ctrl->free_block_head;
    block ->next = next_addr;
    release(pool,block_addr);

    block = (HashMapBlock*)get_page(pool,next_addr);
    block_addr = next_addr;
    next_addr = block->next;
    block->table[block->n_items++] = addr;
    block->next = 0;
    release(pool,block_addr);

    ctrl->free_block_head = next_addr;
    release(pool,(size / HASH_MAP_DIR_BLOCK_SIZE + 1) * PAGE_SIZE);
    release(pool,0);
    return;
}

off_t hash_table_pop_lower_bound(BufferPool *pool, short size) {
    //print_hash_table(pool);
    //printf("now in hash lower bound!. to find %d.   ",size);
    //my_test_print("now in hash lower\n");

    HashMapControlBlock *ctrl = (HashMapControlBlock*)get_page(pool, 0);
    HashMapDirectoryBlock *dir_block;
    off_t block_addr,next_addr;
    HashMapBlock *block;
    off_t ans = -1;

    for(short i=size;i<ctrl->max_size;i++){
        dir_block = (HashMapDirectoryBlock*)get_page(pool, (i / HASH_MAP_DIR_BLOCK_SIZE + 1) * PAGE_SIZE);
        block_addr = dir_block->directory[i % HASH_MAP_DIR_BLOCK_SIZE];

        if(block_addr != 0){
            block = (HashMapBlock*)get_page(pool,block_addr);
            next_addr = block->next;
            while(next_addr!=0){
                release(pool,block_addr);
                block_addr = next_addr;
                block = (HashMapBlock*)get_page(pool,block_addr);
                next_addr = block->next;
            }

            ans = block->table[block->n_items-1];
            release(pool,block_addr);
            release(pool,(i / HASH_MAP_DIR_BLOCK_SIZE + 1) * PAGE_SIZE);
            release(pool,0);
            hash_table_pop(pool, i, ans);
            printf("true size is %d.\n",i);
            return ans;
        }
        release(pool, (i / HASH_MAP_DIR_BLOCK_SIZE + 1) * PAGE_SIZE);
    }
    release(pool, 0);
    return ans;
}

void hash_table_pop(BufferPool *pool, short size, off_t addr) {
    //print_hash_table(pool);
    //printf("now in hash pop! target is size(%lld) = %d\n", addr, size);
    //my_test_print("now in hash pop\n");

    HashMapControlBlock *ctrl = (HashMapControlBlock*)get_page(pool, 0);
    off_t block_addr = 0, tmp_addr = 0;
    off_t next_addr = 0, pre_addr = 0,last_addr = 0;
    short location = -1;
    HashMapBlock *block = NULL;
    HashMapBlock *tmp_block = NULL;

    if(size >= ctrl->max_size || size < 0){
        release(pool, 0);
        printf("hash_pop failed! invalid size : %d.\n",size);
        return;
    }
    if(addr<0){
        release(pool, 0);
        printf("hash_pop failed! invalid addr !\n");
        return;
    }

    HashMapDirectoryBlock *dir_block =
        (HashMapDirectoryBlock*)get_page(pool, (size / HASH_MAP_DIR_BLOCK_SIZE + 1) * PAGE_SIZE);
    block_addr = dir_block->directory[size % HASH_MAP_DIR_BLOCK_SIZE];

    if(block_addr==0){
        printf("hash_pop failed! size no find.\n");
        release(pool, (size / HASH_MAP_DIR_BLOCK_SIZE + 1) * PAGE_SIZE);
        release(pool,0);
        return;
    }
    block = (HashMapBlock*)get_page(pool,block_addr);
    next_addr = block->next;
    while(next_addr!=0){
        pre_addr = block_addr;
        block_addr = next_addr;
        for(short i=0;i<block->n_items;i++){
            if(block->table[i]==addr){
                location = i;
                tmp_addr = pre_addr;
            }
        }
        release(pool,pre_addr);
        block = (HashMapBlock*)get_page(pool,block_addr);
        next_addr = block->next;
    }
    for(short i=0;i<block->n_items;i++){
        if(block->table[i]==addr){
            location = i;
            tmp_addr = block_addr;
        }
    }
    last_addr = block->table[block->n_items-1];
    if(location==-1){
        printf("hash pop failed! no addr.\n");
    }
    if(tmp_addr != block_addr){
        tmp_block = (HashMapBlock*)get_page(pool, tmp_addr);
        tmp_block->table[location] = last_addr;
        release(pool, tmp_addr);
    }else{
        block->table[location] = block->table[block->n_items-1];
    }
    block->table[--block->n_items] = 0;
    
    
    if(block->n_items==0){
        block->next = ctrl->free_block_head;
        ctrl->free_block_head = block_addr;
        release(pool,block_addr);
        if(pre_addr==0){
            dir_block ->directory[size % HASH_MAP_DIR_BLOCK_SIZE] = 0;
            release(pool,(size / HASH_MAP_DIR_BLOCK_SIZE + 1) * PAGE_SIZE);
            release(pool,0);
            //printf("hash pop success!");
            return;
        }
        block = (HashMapBlock*) get_page(pool,pre_addr);
        block->next = 0;
        release(pool,pre_addr);
        release(pool,(size / HASH_MAP_DIR_BLOCK_SIZE + 1) * PAGE_SIZE);
        release(pool,0);
        //printf("hash pop success!");
        return;
    }
    release(pool,block_addr);
    release(pool,(size / HASH_MAP_DIR_BLOCK_SIZE + 1) * PAGE_SIZE);
    release(pool,0);
    //printf("hash pop success!");
    return;
}

void print_hash_table(BufferPool *pool) {
    HashMapControlBlock *ctrl = (HashMapControlBlock*)get_page(pool, 0);
    HashMapDirectoryBlock *dir_block;
    off_t block_addr, next_addr;
    HashMapBlock *block;
    int i, j;
    printf("----------HASH TABLE----------\n");
    for (i = 0; i < ctrl->max_size; ++i) {
        dir_block = (HashMapDirectoryBlock*)get_page(pool, (i / HASH_MAP_DIR_BLOCK_SIZE + 1) * PAGE_SIZE);
        if (dir_block->directory[i % HASH_MAP_DIR_BLOCK_SIZE] != 0) {
            printf("%d:", i);
            block_addr = dir_block->directory[i % HASH_MAP_DIR_BLOCK_SIZE];
            while (block_addr != 0) {
                block = (HashMapBlock*)get_page(pool, block_addr);
                printf("  [" FORMAT_OFF_T "]", block_addr);
                printf("{");
                for (j = 0; j < block->n_items; ++j) {
                    if (j != 0) {
                        printf(", ");
                    }
                    printf(FORMAT_OFF_T, block->table[j]);
                }
                printf("}");
                next_addr = block->next;
                release(pool, block_addr);
                block_addr = next_addr;
            }
            printf("\n");
        }
        release(pool, (i / HASH_MAP_DIR_BLOCK_SIZE + 1) * PAGE_SIZE);
    }
    release(pool, 0);
    printf("------------------------------\n");
    return;
}