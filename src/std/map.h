#ifndef __MAP_H__
#define __MAP_H__

typedef struct hash_map_cell hash_map_cell, *hash_map_cell_t;
struct hash_map_cell {
  void* data;
  unsigned long key;
  hash_map_cell_t next;
};

hash_map_cell_t hash_map_cell_create(unsigned long key, void *data);
void hash_map_cell_destroy(hash_map_cell_t cell);


typedef struct hash_map{
  hash_map_cell_t *bucket;
  long size;
  long c;
  unsigned long key_c;
} hash_map, *hash_map_t;

hash_map_t hash_map_create(long size);
void hash_map_destroy(hash_map_t hash);
long hash_map_size(hash_map_t hash);
unsigned long hash_map_new_key(hash_map_t hash);
void hash_map_add(hash_map_t hash, unsigned long key, void *data);
void* hash_map_pop(hash_map_t hash, unsigned long key);
int hash_map_remove(hash_map_t hash, void* data);
void* hash_map_find(hash_map_t hash, unsigned long key);
void hash_map_print(hash_map_t hash, void (*func)(const void*) );


#define HASHMAP_MAKE_TYPE_INTERFACE(TYPE) \
typedef hash_map_t TYPE ## _hash_map_t; \
TYPE ## _hash_map_t TYPE ## _hash_map_create(long size); \
void TYPE ## _hash_map_destroy(TYPE ## _hash_map_t hash); \
long TYPE ## _hash_map_size(TYPE ## _hash_map_t hash); \
unsigned long TYPE ## _hash_map_new_key(TYPE ## _hash_map_t hash); \
void TYPE ## _hash_map_add(TYPE ## _hash_map_t hash, unsigned long key, TYPE ## _t data); \
TYPE ## _t TYPE ## _hash_map_pop(TYPE ## _hash_map_t hash, unsigned long key); \
int TYPE ## _hash_map_remove(TYPE ## _hash_map_t hash, TYPE ## _t data); \
TYPE ## _t TYPE ## _hash_map_find(TYPE ## _hash_map_t hash, unsigned long key); \
void TYPE ## _hash_map_print(TYPE ## _hash_map_t hash, void (*func)(const TYPE ## _t) );

#define HASHMAP_MAKE_TYPE_IMPLEMENTATION(TYPE) \
TYPE ## _hash_map_t TYPE ## _hash_map_create(long size){ \
  return (TYPE ## _hash_map_t)hash_map_create(size); \
} \
void TYPE ## _hash_map_destroy(TYPE ## _hash_map_t hash){ \
  hash_map_destroy((hash_map_t)hash); \
} \
long TYPE ## _hash_map_size(TYPE ## _hash_map_t hash){ \
  return hash_map_size((hash_map_t)hash); \
} \
unsigned long TYPE ## _hash_map_new_key(TYPE ## _hash_map_t hash){ \
  return hash_map_new_key((hash_map_t)hash); \
} \
void TYPE ## _hash_map_add(TYPE ## _hash_map_t hash, unsigned long key, TYPE ## _t data){ \
  hash_map_add((hash_map_t)hash, key, (void*)data); \
} \
TYPE ## _t TYPE ## _hash_map_pop(TYPE ## _hash_map_t hash, unsigned long key){ \
  return (TYPE ## _t)hash_map_pop((hash_map_t)hash, key); \
} \
int TYPE ## _hash_map_remove(TYPE ## _hash_map_t hash, TYPE ## _t data){ \
  return hash_map_remove((hash_map_t)hash, (void*)data); \
} \
TYPE ## _t TYPE ## _hash_map_find(TYPE ## _hash_map_t hash, unsigned long key){ \
  return (TYPE ## _t)hash_map_find((hash_map_t)hash, key); \
} \
void TYPE ## _hash_map_print(TYPE ## _hash_map_t hash, void (*func)(const TYPE ## _t) ){ \
  hash_map_print((hash_map_t)hash, (void (*)(const void*))func); \
}

#endif // __MAP_H__
