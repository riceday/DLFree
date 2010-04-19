#include <assert.h>

#include "uset.h"
#include "std.h"

static int
Max(int x, int y){
  return x > y ? x : y;
}


HASHMAP_MAKE_TYPE_IMPLEMENTATION(union_set_cell)
VECTOR_MAKE_TYPE_IMPLEMENTATION(union_set_cell)
     
union_set_cell_t
union_set_cell_create(const void* data, int set){
  union_set_cell_t cell = std_malloc(sizeof(union_set_cell));

  cell -> data = data;
  cell -> parent = NULL;

  cell -> set = set;
  cell -> height = 0;
  return cell;
}

void
union_set_cell_destroy(union_set_cell_t cell){
  std_free(cell);
}

union_set_t
union_set_create(){
  union_set_t uset = std_malloc(sizeof(union_set));
  uset -> union_set_cell_map = union_set_cell_hash_map_create(UNION_SET_HASH_MAP_SIZE);
  uset -> union_set_cell_vec = union_set_cell_vector_create(1);
  uset -> nset = 0;
  return uset;
}

void
union_set_destroy(union_set_t uset){
  union_set_cell_t cell;
  union_set_cell_hash_map_destroy(uset -> union_set_cell_map);

  /* clearn up union_set_cells */
  while(union_set_cell_vector_size(uset -> union_set_cell_vec)){
    cell = union_set_cell_vector_pop(uset -> union_set_cell_vec);
    union_set_cell_destroy(cell);
  }
  union_set_cell_vector_destroy(uset -> union_set_cell_vec);
  
  std_free(uset);
}

void
union_set_add(union_set_t uset, const void *data){
  int set = uset -> nset ++;
  union_set_cell_t cell = union_set_cell_create(data, set);

  unsigned long key = (unsigned long) data; /* forcefully create key from data */
  union_set_cell_hash_map_add(uset -> union_set_cell_map, key, cell);
  union_set_cell_vector_add(uset -> union_set_cell_vec, cell);
}

union_set_cell_t
union_set_find_set_parent(union_set_t uset, const void *data){
  unsigned long key = (unsigned long) data;
  union_set_cell_t cell = union_set_cell_hash_map_find(uset -> union_set_cell_map, key);
  assert(cell != NULL);

  /* traverse cell tree to top parent */
  while(cell -> parent != NULL)
    cell = cell -> parent;

  return cell;
}

int
union_set_get_set(union_set_t uset, const void *data){
  union_set_cell_t top = union_set_find_set_parent(uset, data);
  return top -> set;
}

int
union_set_is_same_set(union_set_t uset, const void *d0, const void *d1){
  union_set_cell_t top0 = union_set_find_set_parent(uset, d0);
  union_set_cell_t top1 = union_set_find_set_parent(uset, d1);

  /* 2 items are in same set, only if top parents are same */
  return top0 == top1;
}

void
union_set_merge_set(union_set_t uset, const void *d0, const void *d1){
  union_set_cell_t top0 = union_set_find_set_parent(uset, d0);
  union_set_cell_t top1 = union_set_find_set_parent(uset, d1);

  assert(top0 != top1);
  /* to merge make one top parent, child of another */
  /* be sure to make shallower tree parent child */
  if(top0 -> height < top1 -> height){
    top0 -> parent = top1;
    top1 -> height = Max(top1 -> height, top0 -> height + 1);
  }else{
    top1 -> parent = top0;
    top0 -> height = Max(top0 -> height, top1 -> height + 1);
  }
}
