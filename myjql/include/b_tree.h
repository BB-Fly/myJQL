#ifndef _B_TREE_H
#define _B_TREE_H

#include "file_io.h"
#include "table.h"

/* it MUST be satisfied that: DEGREE >= 2 */
#define DEGREE (((PAGE_SIZE) - sizeof(size_t) - sizeof(off_t) - sizeof(char) + sizeof(RID)) / 2 / (sizeof(off_t) + sizeof(RID)))

typedef struct {
  /* B-Tree Node */
  /* you can modify anything in this struct */
  size_t n;
  off_t next;
  off_t child[2 * DEGREE];
  RID row_ptr[2 * DEGREE];
  char leaf;
} BNode;

typedef struct {
  /* B-Tree Control Block */
  /* you can modify anything in this struct */
  off_t root_node;
  off_t free_node_head;
  off_t last_addr;
} BCtrlBlock;

/* BEGIN: --------------------------------- DO NOT MODIFY! --------------------------------- */

typedef int (*b_tree_row_row_cmp_t)(RID, RID);
typedef int (*b_tree_ptr_row_cmp_t)(void *, size_t, RID);
typedef RID (*b_tree_insert_nonleaf_handler_t)(RID rid);
typedef void (*b_tree_delete_nonleaf_handler_t)(RID rid);

void b_tree_init(const char *filename, BufferPool *pool);

void b_tree_close(BufferPool *pool);

RID b_tree_search(BufferPool *pool, void *key, size_t size, b_tree_ptr_row_cmp_t cmp);

RID b_tree_insert(BufferPool *pool, RID rid, b_tree_row_row_cmp_t cmp, b_tree_insert_nonleaf_handler_t fun);

void b_tree_delete(BufferPool *pool, RID rid, b_tree_row_row_cmp_t cmp, b_tree_insert_nonleaf_handler_t insert_handler, b_tree_delete_nonleaf_handler_t delete_handler);

void RecursiveTravel(BufferPool *pool, off_t T, int Level);

/* END:   --------------------------------- DO NOT MODIFY! --------------------------------- */

#endif  /* _B_TREE_H */