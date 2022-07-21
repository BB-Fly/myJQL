#include "str.h"

#include "table.h"

void read_string(Table *table, RID rid, StringRecord *record) {
    StringChunk *chunk = &record->chunk;
    //printf("now in read string. \n");
    //my_test_print("\tSTR now in read string\n");
    table_read(table, rid, (ItemPtr)chunk);
    record->idx = 0;
    rid = get_str_chunk_rid(&chunk);
    //my_test_p_num(get_rid_block_addr(rid));
    return;
}

int has_next_char(StringRecord *record) {
    StringChunk *chunk = &record->chunk;
    short idx = record->idx;
    short len = get_str_chunk_size(chunk);
    if(idx < len){
        return 1;
    }
    RID rid = get_str_chunk_rid(chunk);
    off_t block_addr = get_rid_block_addr(rid);
    return block_addr >= 0;
}

char next_char(Table *table, StringRecord *record) {
    StringChunk *chunk = &record->chunk;
    short len = get_str_chunk_size(chunk);
    if(record->idx >= len){
        RID rid = get_str_chunk_rid(chunk);
        //my_test_print("\tnow in next char jump!\n");
        //my_test_p_num(get_rid_block_addr(rid));
        //my_test_p_num(get_rid_idx(rid));
        off_t block_addr = get_rid_block_addr(rid);
        short index = get_rid_idx(rid);
        Block *block = (Block*)get_page(&table->data_pool, block_addr);

        ItemPtr item = get_item(block, index);
        ItemID item_id = get_item_id(block,index);
        short item_size = get_item_id_size(item_id);
        //my_test_print("\tnow in next char\n");
        for(short i = 0; i<item_size; i++){
            chunk->data[i] = item[i];
        }
        record->idx = 0;
        release(&table->data_pool, block_addr);
    }
    return get_str_chunk_data_ptr(chunk)[record->idx++];
}

int compare_string_record(Table *table, const StringRecord *a, const StringRecord *b) {
    StringRecord A = *a, B = *b;
    while(1){
        if(has_next_char(&A)){
            if(has_next_char(&B)){
                char _a = next_char(table, &A);
                char _b = next_char(table, &B);
                if(_a==_b){
                    continue;
                }else{
                    return _a - _b;
                }
            }else{
                return (int)next_char(table,&A);
            }
        }else{
            if(has_next_char(&B)){
                return (int)next_char(table,&B);
            }else{
                return 0;
            }
        }
    }
}

RID write_string(Table *table, const char *data, off_t size) {
    //printf("now in write string. \n");
    //my_test_print("\tSTR now in write string\n");
    StringChunk chunk;
    RID rid;
    get_rid_block_addr(rid) = -1;
    get_rid_idx(rid) = 0;

    if(size<=0){
        get_str_chunk_rid(&chunk) = rid;
        get_str_chunk_size(&chunk) = 0;
        rid = table_insert(table,(ItemPtr)&chunk, calc_str_chunk_size(0));
        return rid;
    }

    off_t num = size<=0 ? 0 : (size-1) / STR_CHUNK_MAX_LEN;
    short tmp = size % STR_CHUNK_MAX_LEN;
    short remainder = (size!=0 && tmp==0) ? STR_CHUNK_MAX_LEN : tmp;

    get_str_chunk_rid(&chunk) = rid;
    get_str_chunk_size(&chunk) = remainder;
    for(short i=0; i<remainder; i++){
        get_str_chunk_data_ptr(&chunk)[i] = data[i + num * STR_CHUNK_MAX_LEN];
    }

    rid = table_insert(table, (ItemPtr)&chunk, calc_str_chunk_size(remainder));

    while(num--){
        get_str_chunk_rid(&chunk) = rid;
        get_str_chunk_size(&chunk) = STR_CHUNK_MAX_LEN;
        for(short i=0;i<STR_CHUNK_MAX_LEN;i++){
            get_str_chunk_data_ptr(&chunk)[i] = data[i + num * STR_CHUNK_MAX_LEN];
        }
        rid = table_insert(table, (ItemPtr)&chunk, STR_CHUNK_MAX_SIZE);
    }

    return rid;

}

void delete_string(Table *table, RID rid) {
    //printf("now in delete string. \n");
    //my_test_print("\tSTR now in delete string\n");
    StringChunk chunk;
    while(get_rid_block_addr(rid)>=0){
        //my_test_p_num(get_rid_block_addr(rid));
        //my_test_p_num(get_rid_idx(rid));
        table_read(table,rid, (ItemPtr)&chunk);
        table_delete(table, rid);
        rid = get_str_chunk_rid(&chunk);
    }
    return;
}

// void print_string(Table *table, const StringRecord *record) {
//     StringRecord rec = *record;
//     printf("\"");
//     while (has_next_char(&rec)) {
//         printf("%c", next_char(table, &rec));
//     }
//     printf("\"");
// }

size_t load_string(Table *table, const StringRecord *record, char *dest, size_t max_size) {
    //my_test_print("\tSTR now in load string\n");
    StringRecord re = *record;
    size_t i = 0;
    while(has_next_char(&re) && i <max_size){
        dest[i++] = next_char(table, &re);
    }
    dest[i] = '\0';
    return i;
}

// void chunk_printer(ItemPtr item, short item_size) {
//     if (item == NULL) {
//         printf("NULL");
//         return;
//     }
//     StringChunk *chunk = (StringChunk*)item;
//     short size = get_str_chunk_size(chunk), i;
//     printf("StringChunk(");
//     print_rid(get_str_chunk_rid(chunk));
//     printf(", %d, \"", size);
//     for (i = 0; i < size; i++) {
//         printf("%c", get_str_chunk_data_ptr(chunk)[i]);
//     }
//     printf("\")");
// }