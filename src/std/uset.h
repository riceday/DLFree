#ifndef __UNION_SET_H__
#define __UNION_SET_H__

#include "vector.h"
#include "map.h"

/*
Union Set data-structure:
a structure where each data item initially belongs to its unique set
sets may be merged together, and items in each set will
belong to the same set

major operations are:
add: add a new data item to the union set (create a new unique set)
is_same: see if 2 data items belong to the same set
merge: merge 2 sets to which the given items belong

complexity:
all operations can be achieved in O(log n), where n
is the number of items in the union set
add: O(1)
is_same: O(log n)
merge: O(log n)

*/

typedef struct union_set_cell union_set_cell, *union_set_cell_t;

#define UNION_SET_HASH_MAP_SIZE 100

struct union_set_cell{
  const void* data;
  union_set_cell_t parent;

  /* below info make sense only for authority */
  int set;
  int height;
};

HASHMAP_MAKE_TYPE_INTERFACE(union_set_cell)
VECTOR_MAKE_TYPE_INTERFACE(union_set_cell)
     
typedef struct union_set{
  union_set_cell_hash_map_t union_set_cell_map;
  union_set_cell_vector_t union_set_cell_vec; /* just for ref-count */
  
  int nset;
} union_set, *union_set_t;

union_set_t union_set_create();
void union_set_destroy(union_set_t set);
void union_set_add(union_set_t set, const void *data);
int union_set_get_set(union_set_t set, const void *data);
int union_set_is_same_set(union_set_t set, const void *d0, const void *d1);
void union_set_merge_set(union_set_t set, const void *d0, const void *d1);

#define UNIONSET_MAKE_TYPE_INTERFACE(TYPE) \
typedef union_set_t TYPE ## _union_set_t; \
TYPE ## _union_set_t TYPE ## _union_set_create(); \
void TYPE ## _union_set_destroy(TYPE ## _union_set_t set); \
void TYPE ## _union_set_add(TYPE ## _union_set_t set, const TYPE ## _t data); \
int TYPE ## _union_set_get_set(TYPE ## _union_set_t set, const TYPE ## _t data); \
int TYPE ## _union_set_is_same_set(TYPE ## _union_set_t set, const TYPE ## _t d0, const TYPE ## _t d1); \
void TYPE ## _union_set_merge_set(TYPE ## _union_set_t set, const TYPE ## _t d0, const TYPE ## _t d1);

#define UNIONSET_MAKE_TYPE_IMPLEMENTATION(TYPE) \
TYPE ## _union_set_t TYPE ## _union_set_create(){ \
  return (TYPE ## _union_set_t)union_set_create(); \
} \
void TYPE ## _union_set_destroy(TYPE ## _union_set_t set){ \
  union_set_destroy((union_set_t)set); \
} \
void TYPE ## _union_set_add(TYPE ## _union_set_t set, const TYPE ## _t data){ \
  union_set_add((union_set_t)set, (const void*)data); \
} \
int TYPE ## _union_set_get_set(TYPE ## _union_set_t set, const TYPE ## _t data){ \
  return union_set_get_set((union_set_t)set, (const void*)data); \
} \
int TYPE ## _union_set_is_same_set(TYPE ## _union_set_t set, const TYPE ## _t d0, const TYPE ## _t d1){ \
  return union_set_is_same_set((union_set_t)set, (const void*)d0, (const void*)d1); \
} \
void TYPE ## _union_set_merge_set(TYPE ## _union_set_t set, const TYPE ## _t d0, const TYPE ## _t d1){ \
  union_set_merge_set((union_set_t)set, (const void*)d0, (const void*)d1); \
}

#endif // __UNION_SET_H__
