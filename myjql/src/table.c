#include "table.h"

#include "hash_map.h"

#include <stdio.h>

void table_init(Table *table, const char *data_filename, const char *fsm_filename) {
    init_buffer_pool(data_filename, &table->data_pool);
    hash_table_init(fsm_filename, &table->fsm_pool, PAGE_SIZE / HASH_MAP_DIR_BLOCK_SIZE);
}

void table_close(Table *table) {
    close_buffer_pool(&table->data_pool);
    hash_table_close(&table->fsm_pool);
}

off_t table_get_total_blocks(Table *table) {
    return table->data_pool.file.length / PAGE_SIZE;
}

short table_block_get_total_items(Table *table, off_t block_addr) {
    Block *block = (Block*)get_page(&table->data_pool, block_addr);
    short n_items = block->n_items;
    release(&table->data_pool, block_addr);
    return n_items;
}

void table_read(Table *table, RID rid, ItemPtr dest) {
    //printf("TABLE now in table read! target is: ");
    //my_test_print("    TABLE now in table read\n");
    //print_rid(rid);
    //printf("\n");
    off_t block_addr = get_rid_block_addr(rid);
    short idx = get_rid_idx(rid);
    Block *block = (Block*)get_page(&table->data_pool,block_addr);

    ItemID item_id = get_item_id(block,idx);
    short item_avail = get_item_id_availability(item_id);
    if(item_avail==1){
        printf("table_read failed!, avail is 1.\n");
        //my_test_print("table_read failed!, avail is 1.\n");
        release(&table->data_pool,block_addr);
        return;
    }

    short item_size = get_item_id_size(item_id);
    ItemPtr tmp = get_item(block,idx);

    for(short i=0;i<item_size;i++){
        dest[i] = tmp[i];
    }
    //my_test_p_num(*((off_t*)tmp));
    //my_test_p_num(*((off_t*)dest));

    release(&table->data_pool,block_addr);
    return;
}

RID table_insert(Table *table, ItemPtr src, short size) {
    //printf("now in table insert!, size is %d\n", size);
    //my_test_print("    TABLE now in table insert\n");

    off_t block_addr = hash_table_pop_lower_bound(&table->fsm_pool,size);
    //printf("addr:%lld has size at least %d.\n",block_addr,size);

    // if(block_addr==-1 && table->data_pool.file.length==0){
    //     Page* page = get_page(&table->data_pool, 0);
    //     init_block((Block*)page);
    // }
    // release(&table->data_pool,0);
    
    Block *block = NULL;
    if(block_addr==-1){
        block_addr = table->data_pool.file.length;
        block = (Block*)get_page(&table->data_pool,block_addr);
        init_block(block);
    }else{
        block = (Block*)get_page(&table->data_pool,block_addr);
    }

    //short pre_size = block_get_size(block);
    short idx = new_item(block,src,size);
    short now_size = block_get_size(block);

    release(&table->data_pool,block_addr);

    // hash_table_pop(&table->fsm_pool,pre_size,block_addr);
    if(now_size>=0) hash_table_insert(&table->fsm_pool,now_size,block_addr);

    //release(&table->data_pool,block_addr);

    RID rid;
    get_rid_block_addr(rid) = block_addr;
    get_rid_idx(rid) = idx;
    // print_rid(rid);
    // printf(" rid is returned!\n");
    // printf("now the size is %d\n",now_size);
    //my_test_p_num(block_addr);
    //my_test_p_num(idx);
    return rid;
}

void table_delete(Table *table, RID rid) {
    // printf("now in table delete! target is: ");
    // //my_test_print("    TABLE now in table delete\n");
    // print_rid(rid);
    // printf("\n");
    off_t block_addr = get_rid_block_addr(rid);
    short idx = get_rid_idx(rid);
    Block*block = (Block*)get_page(&table->data_pool,block_addr);

    short pre_size = block_get_size(block);
    delete_item(block,idx);
    short now_size = block_get_size(block);

    if(pre_size>=0) hash_table_pop(&table->fsm_pool,pre_size,block_addr);
    if(now_size>=0) hash_table_insert(&table->fsm_pool,now_size,block_addr);

    release(&table->data_pool,block_addr);
    return;
}

// void print_table(Table *table, printer_t printer) {
//     printf("\n---------------TABLE---------------\n");
//     off_t i, total = table_get_total_blocks(table);
//     off_t block_addr;
//     Block *block;
//     for (i = 0; i < total; ++i) {
//         block_addr = i * PAGE_SIZE;
//         block = (Block*)get_page(&table->data_pool, block_addr);
//         printf("[" FORMAT_OFF_T "]\n", block_addr);
//         print_block(block, printer);
//         release(&table->data_pool, block_addr);
//     }
//     printf("***********************************\n");
//     print_hash_table(&table->fsm_pool);
//     printf("-----------------------------------\n\n");
// }

void print_rid(RID rid) {
    printf("RID(" FORMAT_OFF_T ", %d)", get_rid_block_addr(rid), get_rid_idx(rid));
}

// void analyze_table(Table *table) {
//     block_stat_t stat, curr;
//     off_t i, total = table_get_total_blocks(table);
//     off_t block_addr;
//     Block *block;
//     stat.empty_item_ids = 0;
//     stat.total_item_ids = 0;
//     stat.available_space = 0;
//     for (i = 0; i < total; ++i) {
//         block_addr = i * PAGE_SIZE;
//         block = (Block*)get_page(&table->data_pool, block_addr);
//         analyze_block(block, &curr);
//         release(&table->data_pool, block_addr);
//         accumulate_stat_info(&stat, &curr);
//     }
//     printf("++++++++++ANALYSIS++++++++++\n");
//     printf("total blocks: " FORMAT_OFF_T "\n", total);
//     total *= PAGE_SIZE;
//     printf("total size: " FORMAT_OFF_T "\n", total);
//     printf("occupancy: %.4f\n", 1. - 1. * stat.available_space / total);
//     printf("ItemID occupancy: %.4f\n", 1. - 1. * stat.empty_item_ids / stat.total_item_ids);
//     printf("++++++++++++++++++++++++++++\n\n");
// }