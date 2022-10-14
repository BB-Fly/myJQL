#include "myjql.h"

#include "buffer_pool.h"
#include "b_tree.h"
#include "table.h"
#include "str.h"

typedef struct {
    RID key;
    RID value;
} Record;

void read_record(Table *table, RID rid, Record *record) {
    table_read(table, rid, (ItemPtr)record);
}

RID write_record(Table *table, const Record *record) {
    return table_insert(table, (ItemPtr)record, sizeof(Record));
}

void delete_record(Table *table, RID rid) {
    table_delete(table, rid);
}

BufferPool bp_idx;
Table tbl_rec;
Table tbl_str;

void myjql_init() {
    b_tree_init("rec.idx", &bp_idx);
    table_init(&tbl_rec, "rec.data", "rec.fsm");
    table_init(&tbl_str, "str.data", "str.fsm");
}

void myjql_close() {
    b_tree_close(&bp_idx);
    table_close(&tbl_rec);
    table_close(&tbl_str);
}

RID my_fun(RID rid) {
    return rid;
}
void my_fun_2(RID rid) {
    return;
}

int my_row_row_cmp(RID x, RID y) {

    Record re_x, re_y;
    StringRecord item_x, item_y;

    read_record(&tbl_rec, x, &re_x);
    read_record(&tbl_rec, y, &re_y);

    if (get_rid_block_addr(re_x.key) <= -1) {
        if (get_rid_block_addr(re_y.key) <= -1) {
            printf("rid cmp error: both block_addr = -1\n");
            return 0;
        }
        else {
            printf("rid cmp error: x block_addr = -1\n");
            return -1;
        }

    }
    else if (get_rid_block_addr(re_y.key) <= -1) {
        printf("rid cmp error: y block_addr = -1\n");
        return 1;
    }

    read_string(&tbl_str, re_x.key, &item_x);
    read_string(&tbl_str, re_y.key, &item_y);

    return compare_string_record(&tbl_str, &item_x, &item_y);


}

int my_ptr_row_cmp(char* ptr, size_t len, RID x) {

    Record re_x;
    StringRecord item_x;

    read_record(&tbl_rec, x, &re_x);

    if (get_rid_block_addr(re_x.key) <= -1) {
        printf("ptr_rid cmp error: x block_addr = -1\n");
        return 1;
    }

    read_string(&tbl_str, re_x.key, &item_x);

    size_t i = 0;
    char tmp;

    while (1) {
        if (i >= len) {
            if (has_next_char(&item_x)) {
                tmp = next_char(&tbl_str, &item_x);
                return -1;
            }
            else {
                return 0;
            }
        }
        else {
            if (has_next_char(&item_x)) {
                tmp = next_char(&tbl_str, &item_x);
                if (ptr[i] == tmp) {
                    i++;
                    continue;
                }
                else {
                    return ptr[i] - tmp;
                }
            }
            else {
                return 1;
            }
        }
    }
    return 0;

}

size_t myjql_get(const char *key, size_t key_len, char *value, size_t max_size) {
    RID rid;
    rid = b_tree_search(&bp_idx, key, key_len, my_ptr_row_cmp);
    if (get_rid_block_addr(rid) <= -1) {
        return -1;
    }

    Record record;
    read_record(&tbl_rec, rid, &record);

    StringRecord str_record;
    read_string(&tbl_str, record.value, &str_record);

    return load_string(&tbl_str, &str_record, value, max_size);
}

void myjql_set(const char *key, size_t key_len, const char *value, size_t value_len) {
    Record record;
    RID rid;

    rid = b_tree_search(&bp_idx, key, key_len, my_ptr_row_cmp);
    if (get_rid_block_addr(rid) <= -1) {

        record.key = write_string(&tbl_str, key, key_len);
        record.value = write_string(&tbl_str, value, value_len);

        rid = write_record(&tbl_rec, &record);

        b_tree_insert(&bp_idx, rid, my_row_row_cmp, my_fun);
        return;
    }

    read_record(&tbl_rec, rid, &record);

    delete_string(&tbl_str, record.value);
    record.value = write_string(&tbl_str, value, value_len);

    b_tree_delete(&bp_idx, rid, my_row_row_cmp, my_fun, my_fun_2);
    delete_record(&tbl_rec, rid);

    rid = write_record(&tbl_rec, &record);
    b_tree_insert(&bp_idx, rid, my_row_row_cmp, my_fun);

    return;

}

void myjql_del(const char *key, size_t key_len) {

    RID rid;
    Record record;

    rid = b_tree_search(&bp_idx, key, key_len, my_ptr_row_cmp);
    if (get_rid_block_addr(rid) == -1) {
        return;
    }
    b_tree_delete(&bp_idx, rid, my_row_row_cmp, my_fun, my_fun_2);

    read_record(&tbl_rec, rid, &record);
    delete_record(&tbl_rec, rid);

    delete_string(&tbl_str, record.key);
    delete_string(&tbl_str, record.value);

    return;

}

/* void myjql_analyze() {
    printf("Record Table:\n");
    analyze_table(&tbl_rec);
    printf("String Table:\n");
    analyze_table(&tbl_str);
} */