#include "block.h"

#include <stdio.h>
#include <string.h>

void init_block(Block *block) {
    memset(block, 0, sizeof(Block));
    block->n_items = 0;
    block->head_ptr = 3 * sizeof(short);
    block->tail_ptr = (short)sizeof(Block);
    return;
}

ItemPtr get_item(Block *block, short idx) {
    //my_test_print("BLOCK now in get item\n");
    if (idx < 0 || idx >= block->n_items) {
        printf("get item error: idx is out of range\n");
        //my_test_print("get item error: idx is out of range\n");
        return NULL;
    }
    ItemID item_id = get_item_id(block, idx);
    if (get_item_id_availability(item_id)) {
        printf("get item error: item_id is not used\n");
        //my_test_print("get item error: item_id is not used\n");
        return NULL;
    }
    short offset = get_item_id_offset(item_id);
    //my_test_p_num(*((off_t*)((char*)block+offset)));
    return (char*)block + offset;
}

short new_item(Block *block, ItemPtr item, short item_size) {
    //printf("now in new item, item_size is %d, room size is %d\n",item_size,block_get_size(block));
    // my_test_print("BLOCK now in new item\n");
    // my_test_p_num(*(off_t*)item);
    if(block->head_ptr==0){
        init_block(block);
    }

    for(int i=0;i<block->n_items;i++){
        ItemID item_id = get_item_id(block,i); 

        if(get_item_id_availability(item_id)==1){
            if(block->tail_ptr-block->head_ptr>=item_size){
                block->tail_ptr -= item_size;

                get_item_id(block, i) = compose_item_id(0,block->tail_ptr,item_size);
                for(int j=0;j<item_size;j++){
                    block->data[block->tail_ptr+j-3*sizeof(short)] = item[j];
                }
                //printf("now, the n_item is %d,the room size is %d\n",block->n_items, block_get_size(block));
                return i;
            }
            printf("new_item failed! over size.\n");
            return -1;
        }
    }
    if(block->tail_ptr-block->head_ptr>=sizeof(ItemID)+item_size){
        block->n_items++;
        block->head_ptr += sizeof(ItemID);
        block->tail_ptr -= item_size;
        get_item_id(block,block->n_items-1) = compose_item_id(0,block->tail_ptr,item_size);
        for(int j=0;j<item_size;j++){
            block->data[block->tail_ptr+j-3*sizeof(short)] = item[j];
        }
        //printf("now, the n_item is %d,the room size is %d\n",block->n_items, block_get_size(block));
        return block->n_items-1;
    }
    printf("new_item failed! over size.\n");
    return -1;
}

void delete_item(Block *block, short idx) {
    //printer_t a = &str_printer;
    //print_block(block,a);
    //my_test_print("BLOCK now in delete item\n");
    if (idx < 0 || idx >= block->n_items) {
        printf("delete item error: idx is out of range\n");
        return ;
    }

    ItemID item_id = get_item_id(block,idx);

    if(get_item_id_availability(item_id)==1){
        printf("delete item error: idx is un_available\n");
        return;
    }

    short delta = get_item_id_size(item_id);
    short mark = get_item_id_offset(item_id);
    short end = mark + delta;

    //printf("now in delete item, item_size is %d, room size is %d\n",delta,block_get_size(block));

    for(short i=end-1-delta;i>=block->tail_ptr;i--){
        block->data[i+delta-3*sizeof(short)] = block->data[i-3*sizeof(short)];
    }
    block->tail_ptr += delta;
    item_id = compose_item_id(1,0,0);
    get_item_id(block, idx) = item_id;

    for(short i = block->n_items-1;i>=0;i--){
        item_id = get_item_id(block,i);
        if(get_item_id_availability(item_id)==1){
            block->head_ptr -= sizeof(ItemID);
            block->n_items--;
        }else{
            break;
        }
    }

    for(short i=0;i<block->n_items;i++){
        item_id = get_item_id(block,i);
        short offset = get_item_id_offset(item_id);
        short size = get_item_id_size(item_id);
        short a = get_item_id_availability(item_id);
        if(a==0&&offset<mark){
            get_item_id(block,i) = compose_item_id(0, offset+delta, size);
        }
    }
    //print_block(block,a);
    //printf("now, the n_item is %d,the room size is %d\n",block->n_items, block_get_size(block));
    return;
}

short block_get_size(Block* block){
    if(block->head_ptr==0){
        init_block(block);
    }
    ItemID item_id;
    short is_availd,i;
    for(i = 0;i<block->n_items;i++){
        item_id = get_item_id(block,i);
        is_availd = get_item_id_availability(item_id);
        if(is_availd==1){
            return block->tail_ptr - block->head_ptr;
        }
    }
    short ans = block->tail_ptr - block->head_ptr - sizeof(ItemID);
    return ans >= 0 ? ans:-1;
}

// void str_printer(ItemPtr item, short item_size) {
//     if (item == NULL) {
//         printf("NULL");
//         return;
//     }
//     short i;
//     printf("\"");
//     for (i = 0; i < item_size; ++i) {
//         printf("%c", item[i]);
//     }
//     printf("\"");
// }

// void print_block(Block *block, printer_t printer) {
//     short i, availability, offset, size;
//     ItemID item_id;
//     ItemPtr item;
//     printf("----------BLOCK----------\n");
//     printf("total = %d\n", block->n_items);
//     printf("head = %d\n", block->head_ptr);
//     printf("tail = %d\n", block->tail_ptr);
//     for (i = 0; i < block->n_items; ++i) {
//         item_id = get_item_id(block, i);
//         availability = get_item_id_availability(item_id);
//         offset = get_item_id_offset(item_id);
//         size = get_item_id_size(item_id);
//         if (!availability) {
//             item = get_item(block, i);
//         } else {
//             item = NULL;
//         }
//         printf("%10d%5d%10d%10d\t", i, availability, offset, size);
//         printer(item, size);
//         printf("\n");
//     }
//     printf("-------------------------\n");
// }

// void analyze_block(Block *block, block_stat_t *stat) {
//     short i;
//     stat->empty_item_ids = 0;
//     stat->total_item_ids = block->n_items;
//     for (i = 0; i < block->n_items; ++i) {
//         if (get_item_id_availability(get_item_id(block, i))) {
//             ++stat->empty_item_ids;
//         }
//     }
//     stat->available_space = block->tail_ptr - block->head_ptr 
//         + stat->empty_item_ids * sizeof(ItemID);
// }

// void accumulate_stat_info(block_stat_t *stat, const block_stat_t *stat2) {
//     stat->empty_item_ids += stat2->empty_item_ids;
//     stat->total_item_ids += stat2->total_item_ids;
//     stat->available_space += stat2->available_space;
// }

// void print_stat_info(const block_stat_t *stat) {
//     printf("==========STAT==========\n");
//     printf("empty_item_ids: " FORMAT_SIZE_T "\n", stat->empty_item_ids);
//     printf("total_item_ids: " FORMAT_SIZE_T "\n", stat->total_item_ids);
//     printf("available_space: " FORMAT_SIZE_T "\n", stat->available_space);
//     printf("========================\n");
// }