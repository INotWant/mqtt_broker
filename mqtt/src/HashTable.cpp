//
// 原文：https://blog.csdn.net/smstong/article/details/51145786
// (已做修改)
//

#include <stdlib.h>
#include "HashTable.h"

/* constructor of struct kv */
static void init_kv(struct kv *kv) {
    kv->next = NULL;
    kv->key = NULL;
    kv->value = NULL;
}

/* destructor of struct kv */
static void free_kv(struct kv *kv) {
    if (kv) {
        free(kv);
    }
}

/* new a HashTable instance */
HashTable *hash_table_new() {
    HashTable *ht = (HashTable *)malloc(sizeof(HashTable));
    if (NULL == ht) {
        hash_table_delete(ht);
        return NULL;
    }
    ht->table = (struct kv **)malloc(sizeof(struct kv *) * TABLE_SIZE);
    if (NULL == ht->table) {
        hash_table_delete(ht);
        return NULL;
    }
    memset(ht->table, 0, sizeof(struct kv *) * TABLE_SIZE);
    return ht;
}

/* delete a HashTable instance */
void hash_table_delete(HashTable *ht) {
    if (ht) {
        if (ht->table) {
            int i = 0;
            for (i = 0; i < TABLE_SIZE; i++) {
                struct kv *p = ht->table[i];
                struct kv *q = NULL;
                while (p) {
                    q = p->next;
                    free_kv(p);
                    p = q;
                }
            }
            free(ht->table);
            ht->table = NULL;
        }
        free(ht);
    }
}

/* insert or update a value indexed by key */
int hash_table_put(HashTable *ht, int key, int value) {
    int i = key % TABLE_SIZE;
    struct kv *p = ht->table[i];
    struct kv *prep = p;

    while (p) { /* if key is already stroed, update its value */
        if (p->key == key) {
            p->value = value;
            break;
        }
        prep = p;
        p = p->next;
    }

    if (p == NULL) {/* if key has not been stored, then add it */
        struct kv *kv = (struct kv *)malloc(sizeof(struct kv));
        if (NULL == kv) {
            return -1;
        }
        init_kv(kv);
        kv->next = NULL;
        kv->key = key;
        kv->value = value;

        if (prep == NULL) {
            ht->table[i] = kv;
        } else {
            prep->next = kv;
        }
    }
    return 0;
}

/* get a value indexed by key */
int hash_table_get(HashTable *ht, int key) {
    int i = key % TABLE_SIZE;
    struct kv *p = ht->table[i];
    while (p) {
        if (key == p->key) {
            return p->value;
        }
        p = p->next;
    }
    return NULL;
}

/* remove a value indexed by key */
void hash_table_rm(HashTable *ht, int key) {
    int i = key % TABLE_SIZE;

    struct kv *p = ht->table[i];
    struct kv *prep = p;
    while (p) {
        if (key == p->key) {
            free_kv(p);
            if (p == prep) {
                ht->table[i] = NULL;
            } else {
                prep->next = p->next;
            }
        }
        prep = p;
        p = p->next;
    }
}

/* get a length of hashtable */
int hash_table_len(HashTable *ht){
		int count = 0;
		if (ht) {
        if (ht->table) {
            int i = 0;
            for (i = 0; i < TABLE_SIZE; i++) {
                struct kv *p = ht->table[i];
                while (p) {
                    p = p->next;
                    ++count;
                }
            }
        }
    }
    return count;
}
