//
// 原文：https://blog.csdn.net/smstong/article/details/51145786
// (已做修改)
//

#ifndef MQTT_BROKER_HASHTABLE_H
#define MQTT_BROKER_HASHTABLE_H

#include "Define.h"

//#define TABLE_SIZE (1024*1024)

/* element of the hash table's chain list */
struct kv {
    struct kv *next;
    int key;
    int value;
};

/* HashTable */
typedef struct _HashTable {
    struct kv **table;
} HashTable;

/* new an instance of HashTable */
HashTable *hash_table_new();

/*
delete an instance of HashTable,
all values are removed auotmatically.
*/
void hash_table_delete(HashTable *ht);

/*
add or update a value to ht,
free_value(if not NULL) is called automatically when the value is removed.
return 0 if success, -1 if error occurred.
*/
int hash_table_put(HashTable *ht, int key, int value);

/* get a value indexed by key, return NULL if not found. */
int hash_table_get(HashTable *ht, int key);

/* remove a value indexed by key */
void hash_table_rm(HashTable *ht, int key);

/* get a length of hashtable */
int hash_table_len(HashTable *ht);

#endif //MQTT_BROKER_HASHTABLE_H
