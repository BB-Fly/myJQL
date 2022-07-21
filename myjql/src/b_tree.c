#include "b_tree.h"
#include "buffer_pool.h"

#include <stdio.h>

off_t NewBNode(BufferPool *pool){
    //my_test_print("now in new node.\n");
    BCtrlBlock* ctrl = (BCtrlBlock*)get_page(pool,0);

    BNode* NewNode;
    int i = 0;

    off_t addr = ctrl->free_node_head;

    RID Unavailable;
    get_rid_block_addr(Unavailable) = -1;
    get_rid_idx(Unavailable) = 0;

    NewNode = (BNode*)get_page(pool, addr);

    if(addr < ctrl->last_addr){
        ctrl->free_node_head = NewNode->next;
    }else{
        ctrl->free_node_head += PAGE_SIZE;
        ctrl->last_addr += PAGE_SIZE;
    }

    i = 0;

    while (i < 2 * DEGREE){
        NewNode->row_ptr[i] = Unavailable;
        NewNode->child[i] = 0;
        i++;
    }

    NewNode->next = 0;
    NewNode->n = 0;

    release(pool, addr);
    release(pool, 0);
    return addr;
}

void DeleteNode(BufferPool *pool, off_t addr){
    //my_test_print("now in delete node.\n");
    BNode* node = (BNode*)get_page(pool, addr);
    BCtrlBlock* ctrl = (BCtrlBlock*) get_page(pool, 0);

    node->next = ctrl->free_node_head;
    ctrl->free_node_head = addr;

    release(pool, 0);
    release(pool, addr);
    my_test_print("now out delete node.\n");
    return;
}

off_t Find_Most_Left(BufferPool *pool, off_t addr){
    //my_test_print("now in find left.\n");
    off_t block_addr = addr;
    BNode* block;

    while(block_addr > 0){
        block = (BNode*)get_page(pool,block_addr);
        if(block->child[0]!=0){
            off_t tmp = block_addr;
            block_addr = block->child[0];
            release(pool,tmp);
        }else{
            release(pool,block_addr);
            break;
        }
    }

    return block_addr;
}

off_t Find_Most_Right(BufferPool *pool, off_t addr){
    //my_test_print("now in find right.\n");
    off_t block_addr = addr;
    BNode* block;

    while(block_addr > 0){
        block = (BNode*)get_page(pool,block_addr);
        if(block->child[block->n-1]!=0){
            off_t tmp = block_addr;
            block_addr = block->child[block->n-1];
            release(pool,tmp);
        }else{
            release(pool,block_addr);
            break;
        }
    }

    return block_addr;
}

off_t Find_Sibling(BufferPool *pool, off_t parent, int i){
    //my_test_print("now in find sibling.\n");
    off_t sibling = 0;
    int limit = 2*DEGREE-1;
    BNode* p_block, *s_block;
    off_t next_addr;

    p_block = (BNode*)get_page(pool, parent);

    if(i==0){
        next_addr = p_block->child[1];
        if (next_addr == 0) {
            release(pool, parent);
            return 0;
        }
        s_block = (BNode*)get_page(pool, next_addr);
        if(s_block->n < limit){
            sibling = next_addr;
        }
        release(pool,next_addr);
    }else{
        next_addr=p_block->child[i-1];
        s_block = (BNode*)get_page(pool, next_addr);
        if(s_block->n < limit){
            sibling = next_addr;
            release(pool,next_addr);
        }else{
            release(pool, next_addr);
            if(p_block->n > (i+1)){
                next_addr = p_block->child[i+1];
                s_block = (BNode*)get_page(pool, next_addr);
                if(s_block->n < limit){
                    sibling = next_addr;
                }
                release(pool, next_addr);
            }
        }

    }

    release(pool, parent);
    return sibling;
}

off_t Find_Sibling_2(BufferPool *pool, off_t parent, int i, int* j){
    //my_test_print("now in find sibling2.\n");
    off_t sibling = 0;
    int limit = DEGREE;
    BNode* p_block, *s_block;
    off_t next_addr;

    p_block = (BNode*)get_page(pool, parent);

    if(i==0){
        next_addr = p_block->child[1];
        if (next_addr == 0) {
            release(pool, parent);
            return 0;
        }
        s_block = (BNode*)get_page(pool, next_addr);
        if(s_block->n>limit){
            sibling = next_addr;
            *j = 1;
        }
        release(pool,next_addr);
    }else{
        next_addr=p_block->child[i-1];
        s_block = (BNode*)get_page(pool, next_addr);
        if(s_block->n > limit){
            sibling = next_addr;
            *j = i-1;
            release(pool,next_addr);
        }else{
            release(pool, next_addr);
            if(p_block->n > i+1){
                next_addr = p_block->child[i+1];
                s_block = (BNode*)get_page(pool, next_addr);
                if(s_block->n > limit){
                    sibling = next_addr;
                    *j = i+1;
                }
                release(pool, next_addr);
            }
        }

    }

    release(pool, parent);
    return sibling;
}

off_t InsertKey(BufferPool *pool, int isKey, off_t parent, off_t X, RID Key, int i, int j){
    //my_test_print("now in insert key.\n");
    int k;

    BNode *p_block, *n_block; 
    BNode *x_block = (BNode*)get_page(pool, X);
    off_t next_addr;

    if(isKey){
        k = x_block->n-1;
        while(k>=j){
            x_block->row_ptr[k+1] = x_block->row_ptr[k];
            k--;
        }
        x_block->row_ptr[j] = Key;

        if(parent!=0){
            p_block= (BNode*)get_page(pool, parent);
            p_block->row_ptr[i] = x_block->row_ptr[0];
            release(pool, parent);
        }
        x_block->n++;
    }else{
        p_block = (BNode*)get_page(pool, parent);
        if(x_block->child[0]==0){
            if(i>0){
                next_addr = p_block->child[i-1];
                n_block = (BNode*)get_page(pool, next_addr);
                n_block->next = X;
                release(pool, next_addr);
            }
            x_block->next = p_block->child[i];
        }

        k=p_block->n-1;

        while (k>=i){
            p_block->child[k+1] = p_block->child[k];
            p_block->row_ptr[k+1] = p_block->row_ptr[k];
            k--;
        }
        
        p_block->row_ptr[i] = x_block->row_ptr[0];
        p_block->child[i] = X;

        p_block->n++;

        release(pool, parent);
    }
    release(pool, X);
    my_test_print("now out insert key.\n");
    return X;
}

off_t RemoveKey(BufferPool *pool, int isKey, off_t parent, off_t X, int i, int j){
    //my_test_print("now in remove key\n");
    int k, limit;

    BNode *p_block, *n_block; 
    BNode *x_block = (BNode*)get_page(pool, X);
    off_t next_addr;

    RID unavial;
    get_rid_block_addr(unavial) = -1;
    get_rid_idx(unavial) = 0;

    if(isKey){
        limit = x_block->n;

        k = j+1;
        while(k<limit){
            x_block->row_ptr[k-1] = x_block->row_ptr[k];
            k++;
        }

        x_block->row_ptr[x_block->n-1] = unavial;
        if (parent != 0) {
            p_block = (BNode*)get_page(pool, parent);
            p_block->row_ptr[i] = x_block->row_ptr[0];
            release(pool, parent);
        }
        x_block->n--;
    }else{
        p_block = (BNode*)get_page(pool, parent);

        if(x_block->child[0]==0&&i>0){
            next_addr = p_block->child[i-1];
            n_block = (BNode*)get_page(pool, next_addr);
            n_block->next = p_block->child[i+1];
            release(pool, next_addr);
        }

        limit = p_block->n;
        k = i+1;

        while(k<limit){
            p_block->child[k - 1] = p_block->child[k];
            p_block->row_ptr[k - 1] = p_block->row_ptr[k];
            k++;
        }
        p_block->child[p_block->n - 1] = 0;
        p_block->row_ptr[p_block->n - 1] = unavial;

        p_block->n--;
    }
    release(pool, parent);
    release(pool, X);
    return X;
}

off_t MoveKey(BufferPool *pool, off_t src, off_t dst, off_t parent,int i, int n, b_tree_row_row_cmp_t cmp){
    //my_test_print("now in move key.\n");
    RID tmpKey;
    off_t child, t1 = 0, t2 = 0;
    int j, SrcInFront;

    SrcInFront = 0;
    BNode *p_block = (BNode*)get_page(pool, parent); 
    BNode *s_block = (BNode*)get_page(pool,src);
    BNode *d_block = (BNode*)get_page(pool,dst);

    RID unavial;
    get_rid_block_addr(unavial) = -1;
    get_rid_idx(unavial) = 0;

    if(cmp(s_block->row_ptr[0],d_block->row_ptr[0])<0){
        SrcInFront = 1;
    }

    j=0;

    if(SrcInFront){
        if(s_block->child[0]!=0){
            while(j<n){
                int t = s_block->n-1;
                child = s_block->child[t];
                release(pool, src);
                release(pool, parent);
                release(pool, dst);
                RemoveKey(pool, 0, src, child, t, 0);
                InsertKey(pool, 0, dst, child, unavial,0,0);
                j++;
                p_block = (BNode*)get_page(pool, parent); 
                s_block = (BNode*)get_page(pool,src);
                d_block = (BNode*)get_page(pool,dst);
            }
        }else{
            while(j<n){
                int t = s_block->n-1;
                tmpKey = s_block->row_ptr[t];
                release(pool, src);
                release(pool, parent);
                release(pool, dst);
                RemoveKey(pool, 1, parent, src, i, t);
                InsertKey(pool, 1, parent, dst, tmpKey,i+1,0);
                j++;
                p_block = (BNode*)get_page(pool, parent); 
                s_block = (BNode*)get_page(pool,src);
                d_block = (BNode*)get_page(pool,dst);
            }
        }
        p_block->row_ptr[i+1] = d_block->row_ptr[0];
        p_block->row_ptr[i] = s_block->row_ptr[0];
        if(s_block->n>0){
            release(pool, src);
            release(pool, parent);
            release(pool, dst);
            t1 = Find_Most_Right(pool, src);
            t2 = Find_Most_Left(pool, dst);
            p_block = (BNode*)get_page(pool, parent);
            s_block = (BNode*)get_page(pool, src);
            d_block = (BNode*)get_page(pool, dst);
        }
    }else{
        if(s_block->child[0]!=0){
            while(j<n){
                int t = d_block->n;
                child = s_block->child[0];
                release(pool, src);
                release(pool, parent);
                release(pool, dst);
                RemoveKey(pool, 0, src, child, 0, 0);
                InsertKey(pool, 0, dst, child, unavial,t,0);
                j++;
                p_block = (BNode*)get_page(pool, parent); 
                s_block = (BNode*)get_page(pool,src);
                d_block = (BNode*)get_page(pool,dst);
            }
        }else{
            while(j<n){
                int t = d_block->n;
                tmpKey = s_block->row_ptr[0];
                release(pool, src);
                release(pool, parent);
                release(pool, dst);
                RemoveKey(pool, 1, parent, src, i, 0);
                InsertKey(pool, 1, parent, dst, tmpKey,i-1,t);
                j++;
                p_block = (BNode*)get_page(pool, parent); 
                s_block = (BNode*)get_page(pool,src);
                d_block = (BNode*)get_page(pool,dst);
            }
        }
        p_block->row_ptr[i] = s_block->row_ptr[0];
        p_block->row_ptr[i - 1] = d_block->row_ptr[0];

        if(s_block->n>0){
            release(pool, src);
            release(pool, parent);
            release(pool, dst);
            t1 = Find_Most_Right(pool, dst);
            t2 = Find_Most_Left(pool, src);
            p_block = (BNode*)get_page(pool, parent);
            s_block = (BNode*)get_page(pool, src);
            d_block = (BNode*)get_page(pool, dst);
        }
    }
    release(pool, src);
    release(pool, parent);
    release(pool, dst);

    if(t1!=0&&t2!=0){
        s_block = (BNode*)get_page(pool, t1);
        s_block->next = t2;
        release(pool, t1);
    }
    return parent;
}

off_t SplitNode(BufferPool *pool, off_t parent, off_t X, int i){
    //my_test_print("now in split node.\n");
    int j, k, limit;
    off_t new_addr;

    BNode *n_block; 
    BNode *x_block = (BNode*)get_page(pool, X);

    RID unavial;
    get_rid_block_addr(unavial) = -1;
    get_rid_idx(unavial) = 0;

    release(pool, X);
    new_addr = NewBNode(pool);
    x_block = (BNode*)get_page(pool, X);
    n_block = (BNode*)get_page(pool,new_addr);

    k = 0;
    j = x_block->n / 2;
    limit = x_block->n;

    while (j < limit){
        if(x_block->child[0]!=0){
            n_block->child[k] = x_block->child[j];
            x_block->child[j] = 0;
        }
        n_block->row_ptr[k] = x_block->row_ptr[j];
        x_block->row_ptr[j] = unavial;
        n_block->n++;
        x_block->n--;
        j++;
        k++;
    }

    release(pool, X);
    release(pool, new_addr);

    if(parent!=0){
        InsertKey(pool, 0, parent, new_addr, unavial, i+1, 0);
    }else{
        parent = NewBNode(pool);
        InsertKey(pool,0, parent, X, unavial, 0, 0);
        InsertKey(pool,0, parent, new_addr, unavial, 1, 0);
        return parent;
    }
    
    return X;
}

off_t MergeNode(BufferPool *pool, off_t parent, off_t X, off_t S, int i, b_tree_row_row_cmp_t cmp){
    //my_test_print("now in merge node.\n");
    int limit;
    BNode *x_block;
    BNode *s_block = (BNode*)get_page(pool, S);

    if(s_block->n>DEGREE){
        release(pool, S);
        MoveKey(pool, S, X, parent, i, 1, cmp);
    }else{
        x_block = (BNode*)get_page(pool, X);
        limit = x_block->n;
        release(pool, X);
        release(pool, S);
        MoveKey(pool, X, S, parent, i, limit,cmp);
        RemoveKey(pool, 0, parent, X, i, 0);

        DeleteNode(pool, X);

    }
    return parent;
}

off_t B_Insert(BufferPool *pool, off_t T, RID key, int i, off_t parent, b_tree_row_row_cmp_t cmp){
    //my_test_print("now in B insert.\n");
    int j,limit;
    off_t sibling;

    BNode *p_block;
    BNode *t_block = (BNode*)get_page(pool, T);    

    j = 0;
    while(t_block->n>j && cmp(key,t_block->row_ptr[j])>=0){
        if(cmp(key, t_block->row_ptr[j])==0){
            release(pool, T);
            return T;
        }
        j++;
    }
    if(j!=0 && t_block->child[0]!=0){
        j--;
    }

    off_t next_addr = t_block->child[j];
    off_t tmp = t_block->child[0];
    release(pool, T);
    if(tmp==0){
        T = InsertKey(pool, 1, parent, T, key, i, j);
        t_block = (BNode*)get_page(pool, T);
    }else{
        tmp = B_Insert(pool, next_addr, key, j, T, cmp);
        t_block = (BNode* )get_page(pool, T);
        t_block->child[j] = tmp;
    }

    limit = 2*DEGREE-1;

    if(t_block->n > limit){
        release(pool, T);
        if(parent==0){
            T = SplitNode(pool,parent,T,i);
        }else{
            sibling = Find_Sibling(pool, parent, i);
            if(sibling!=0){
                MoveKey(pool, T, sibling, parent, i, 1, cmp);
            }else{
                T = SplitNode(pool, parent, T, i);
            }
        }
        t_block = (BNode*)get_page(pool, T);
    }

    if(parent!=0){
        p_block = (BNode*)get_page(pool, parent);
        p_block->row_ptr[i] = t_block->row_ptr[0];
        release(pool, parent);
    }
    release(pool, T);
    my_test_print("now out b insert.\n");
    return T;
}

off_t B_Remove(BufferPool *pool, off_t T, RID key, int i, off_t parent, b_tree_row_row_cmp_t cmp){
    //printf("B Remove: T=%lld, i = %d, parent=%lld\n",T,i,parent);
    //my_test_print("now in B remove.\n");
    int j, need_adjust;
    off_t tmp, sibling = 0;

    BNode *p_block;
    BNode *t_block = (BNode*)get_page(pool, T);   

    j = 0;

    while(j < t_block->n && cmp(key, t_block->row_ptr[j])>=0){
        if(cmp(key, t_block->row_ptr[j])==0){
            break;
        }
        j++;
    }

    if(t_block->child[0]==0){
        if(cmp(key, t_block->row_ptr[j])!=0 || j == t_block->n){
            release(pool, T);
            return T;
        }
    }else{
        if(j==t_block->n || cmp(key, t_block->row_ptr[j])<0){
            j--;
        }
    }
    if (j < 0) {
        release(pool, T);
        return T;
    }
    off_t t = t_block->child[j];
    tmp = t_block->child[0];
    release(pool, T);
    if(tmp==0){
        T = RemoveKey(pool, 1, parent, T, i, j);
        t_block = (BNode*)get_page(pool, T);
    }else{
        t = B_Remove(pool, t, key, j, T,cmp);
        t_block = (BNode*)get_page(pool, T);
        t_block->child[j] = t;
        if(t!=0){
            BNode* tmp_block = (BNode*) get_page(pool, t);
            t_block->row_ptr[j] = tmp_block->row_ptr[0];
            release(pool, t);
        }
    }

    need_adjust = 0;

    if(parent==0 && t_block->child[0]!=0 && t_block->n <2){
        need_adjust = 1;
    }else if(parent!=0  && t_block->n < DEGREE){
        need_adjust = 1;
    }

    if(need_adjust!=0){
        if(parent==0){
            if(t_block->child[0]!=0 && t_block->n < 2){
                tmp = T;
                T = t_block->child[0];
                release(pool, tmp);
                DeleteNode(pool, tmp);
                return T;
            }
            release(pool,T);
        }else{
            release(pool, T);
            sibling = Find_Sibling_2(pool, parent, i, &j);
            if(sibling!=0){
                MoveKey(pool, sibling, T, parent, j, 1, cmp);
            }else{
                p_block = (BNode*)get_page(pool, parent);
                if(i==0){
                    sibling = p_block->child[1];
                }else{
                    sibling = p_block->child[i-1];
                }
                release(pool, parent);
                parent = MergeNode(pool, parent, T, sibling, i, cmp);
                p_block = (BNode*)get_page(pool, parent);
                T = p_block->child[i];
                release(pool, parent);
            }
        }
    }else{
        release(pool, T);
    }
    return T;

}

RID B_Search(BufferPool *pool, off_t T, void *key, size_t size, b_tree_ptr_row_cmp_t cmp){
    //printf("now in B_SEArch! and T = %lld\n",T);
    //my_test_print("now in B search.\n");
    int j;
    off_t sibling = 0;
    RID rid;
    get_rid_block_addr(rid) = -1;
    get_rid_idx(rid) = 0;

    BNode *t_block = (BNode*)get_page(pool, T);   

    j = 0;

    while(j < t_block->n && cmp(key, size, t_block->row_ptr[j])>=0){
        if(cmp(key, size, t_block->row_ptr[j])==0){
            rid = t_block->row_ptr[j];
            break;
        }
        j++;
    }

    if(t_block->child[0]==0){
        if(j==t_block->n || cmp(key, size, t_block->row_ptr[j])!=0){
            release(pool, T);
            return rid;
        }
    }else{
        if(j==t_block->n || cmp(key, size, t_block->row_ptr[j])<0){
            j--;
        }
    }

    if (j < 0) {
        release(pool, T);
        return rid;
    }

    if (t_block->child[0] == 0) {
        rid = t_block->row_ptr[j];
        release(pool, T);
    }
    else {
        off_t t = t_block->child[j];
        release(pool, T);
        rid = B_Search(pool,t,key, size, cmp);
    }

    return rid;

}

// void RecursiveTravel(BufferPool *pool, off_t T, int Level) {
//     int i;
//     if (T != 0) {
//         printf("%lld:  ",T);
//         printf("[Level:%d]-->", Level);
//         printf("{");
//         i = 0;
//         BNode* block = (BNode*)get_page(pool, T);
//         while (i < block->n)/*  T->Key[i] != Unavailable*/
//             print_rid(block->row_ptr[i++]);
//         printf("}\n");
//         Level++;
//         i = 0;
//         while (i <= block->n) {
//             off_t t = block->child[i];
//             release(pool, T);
//             //printf("i = %d ",i);
//             RecursiveTravel(pool, t, Level);
//             block = (BNode*)get_page(pool, T);
//             i++;
//         }
//         release(pool, T);
//     }else{
//         //printf("T = 0\n");
//     }
//     return;
// }
// void TravelData(BufferPool* pool, off_t T) {
//     off_t Tmp;
//     int i;
//     if (T == 0)
//         return;
//     printf("All Data:");
//     Tmp = T;
//     BNode* block = (BNode*)get_page(pool, Tmp);
//     while (block->child[0] != 0) {
//         off_t t = Tmp;
//         Tmp = block->child[0];
//         release(pool, t);
//         block = (BNode*)get_page(pool, Tmp);
//     }
//     /* ��һƬ��Ҷ */
//     while (Tmp != 0) {
//         i = 0;
//         while (i < block->n)
//             print_rid(block->row_ptr[i++]);
//         off_t t = Tmp;
//         Tmp = block->next;
//         release(pool, t);
//         block = (BNode*)get_page(pool, Tmp);
//     }
//     release(pool, Tmp);
//     printf("\n");
// }

void b_tree_init(const char *filename, BufferPool *pool) {
    //my_test_print("now in b tree init.\n");
    init_buffer_pool(filename, pool);
    /* TODO: add code here */
    if(pool->file.length==0){
        BCtrlBlock* ctrl = (BCtrlBlock*)get_page(pool, 0);
        ctrl->root_node = PAGE_SIZE;
        ctrl->free_node_head = 2*PAGE_SIZE;
        ctrl->last_addr = 128*PAGE_SIZE;

        RID Unavailable;
        get_rid_block_addr(Unavailable) = -1;
        get_rid_idx(Unavailable) = 0;

        BNode* root = (BNode*)get_page(pool,PAGE_SIZE);
        for(short i=0;i<2*DEGREE;i++){
            root->child[i] = 0;
            root->row_ptr[i] = Unavailable;
        }
        root->n=0;
        root->next=0;
        release(pool,PAGE_SIZE);

        BNode* block;
        off_t block_addr = ctrl->free_node_head, next_addr = block_addr+PAGE_SIZE;
        while(block_addr<ctrl->last_addr){
            block = (BNode*)get_page(pool, block_addr);
            block->next = next_addr;
            release(pool, block_addr);
            block_addr = next_addr;
            next_addr += PAGE_SIZE;
        }
        
        release(pool,0);
    }
    return;
}

void b_tree_close(BufferPool *pool) {
    close_buffer_pool(pool);
}

RID b_tree_search(BufferPool *pool, void *key, size_t size, b_tree_ptr_row_cmp_t cmp) {
    //my_test_print("now in b tree search.\n");
    BCtrlBlock *ctrl = (BCtrlBlock*)get_page(pool, 0);
    off_t T = ctrl->root_node;
    release(pool,0);

    RID rid = B_Search(pool, T, key, size, cmp);

    return rid;
}

RID b_tree_insert(BufferPool *pool, RID rid, b_tree_row_row_cmp_t cmp, b_tree_insert_nonleaf_handler_t fun) {

    //my_test_print("now in b tree insert.\n");
    BCtrlBlock *ctrl = (BCtrlBlock*)get_page(pool, 0);
    off_t T = ctrl->root_node;
    release(pool,0);

    T = B_Insert(pool, T, rid, 0, 0, cmp);

    ctrl = (BCtrlBlock*)get_page(pool, 0);
    ctrl->root_node = T;
    release(pool, 0);

    //TravelData(pool, T);
    //RecursiveTravel(pool, T, 0);

    return rid;
}

void b_tree_delete(BufferPool *pool, RID rid, b_tree_row_row_cmp_t cmp, b_tree_insert_nonleaf_handler_t fun, b_tree_delete_nonleaf_handler_t fun2) {
    //my_test_print("now in b tree delete.\n");
    BCtrlBlock *ctrl = (BCtrlBlock*)get_page(pool, 0);
    off_t T = ctrl->root_node;
    release(pool,0);

    T = B_Remove(pool, T, rid, 0, 0, cmp);

    ctrl = (BCtrlBlock*)get_page(pool, 0);
    ctrl->root_node = T;
    release(pool, 0);

    //TravelData(pool, T);
    //RecursiveTravel(pool, T, 0);

    return;
}