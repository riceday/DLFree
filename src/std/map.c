#include <stdio.h>
#include <string.h>
#include "std.h"
#include "map.h"

/****************/
/*   Hash Map   */
/****************/

hash_map_cell_t hash_map_cell_create(unsigned long key, void *data){
  hash_map_cell_t cell = (hash_map_cell_t)std_malloc(sizeof(hash_map_cell));
  cell -> key  = key;
  cell -> data = data;
  cell -> next = NULL;
  return cell;
}

void hash_map_cell_destroy(hash_map_cell_t cell){
  std_free(cell);
}

hash_map_t hash_map_create(long size){
  hash_map_t hash = (hash_map_t) std_malloc(sizeof(hash_map));
  hash -> bucket = (hash_map_cell_t*) std_malloc(sizeof(hash_map_cell_t) * size);
  memset(hash -> bucket, 0, sizeof(hash_map_cell_t) * size);
  hash -> size   = size;
  hash -> c   = 0;
  hash -> key_c = 0;
  return hash;
}

void hash_map_destroy(hash_map_t hash){
  long h;
  hash_map_cell_t cell, next;
  for(h = 0; h < hash -> size; h++){
    cell = hash -> bucket[h];
    while(cell != NULL){
      next = cell -> next;
      hash_map_cell_destroy(cell);
      cell = next;
    }
  }
  std_free(hash -> bucket);
  std_free(hash);
}

long hash_map_size(hash_map_t hash){
  return hash -> c;
}

unsigned long hash_map_new_key(hash_map_t hash){
  return hash -> key_c ++;
}

long hash_map_hash(hash_map_t hash, unsigned long key){
  return key % hash -> size;
/*   long h = key % hash -> size; */
/*   if(h < 0) */
/*     return h + hash -> size; */
/*   else */
/*     return 0; */
}

void hash_map_add(hash_map_t hash, unsigned long key, void *data){
  long h = hash_map_hash(hash, key);
  hash_map_cell_t *cell = hash -> bucket + h;
  hash_map_cell_t new_cell = hash_map_cell_create(key, data);

  new_cell -> next = *cell;
  *cell = new_cell;

  hash -> c++;
}

void* hash_map_pop(hash_map_t hash, unsigned long key){
  void* data;
  long h = hash_map_hash(hash, key);
  hash_map_cell_t cell = hash -> bucket[h];
  hash_map_cell_t prev;
  
  if(cell == NULL)
    return NULL; /* not found */

  if(cell -> key == key){
    data = cell -> data;
    hash -> bucket[h] = cell -> next;
    hash -> c--;
    hash_map_cell_destroy(cell);
    return data;
  }

  for(prev = cell, cell = cell -> next; cell != NULL; prev = cell, cell = cell -> next){
    if(cell -> key == key){
      prev -> next = cell -> next;
      data = cell -> data;
      hash -> c--;
      hash_map_cell_destroy(cell);
      return data;
    }
  }
  
  return NULL; /* not found */
}

int hash_map_remove(hash_map_t hash, void* data){
  long h;
  hash_map_cell_t cell, prev;
  
  for(h = 0; h < hash -> size; h++){
    cell = hash -> bucket[h];

    if(cell == NULL)
      continue; /* not found, go on to next bucket */

    if(cell -> data == data){
      data = cell -> data;
      hash -> bucket[h] = cell -> next;
      hash -> c--;
      hash_map_cell_destroy(cell);
      return 0;
    }
    
    for(prev = cell, cell = cell -> next; cell != NULL; prev = cell, cell = cell -> next){
      if(cell -> data == data){
	prev -> next = cell -> next;
	data = cell -> data;
	hash -> c--;
	hash_map_cell_destroy(cell);
	return 0;
      }
    }
    
  }
  
  return -1; /* not found */
}

void* hash_map_find(hash_map_t hash, unsigned long key){
  long h = hash_map_hash(hash, key);
  hash_map_cell_t cell = hash -> bucket[h];
  while(cell != NULL){
    if(cell -> key == key) /* match */
      return cell -> data;
    cell = cell -> next;
  }
  return NULL; /* not found */
}

void hash_map_print(hash_map_t hash, void (*func)(const void*) ){
  long h;
  hash_map_cell_t cell;
  printf("Hash(%ld) {", hash -> c);
  for(h = 0; h < hash -> size; h++){
    cell = hash -> bucket[h];
    while(cell != NULL){
      //printf("%d(%ld) : ", cell -> key, h);
      printf("%ld : ", cell -> key);
      func(cell -> data);
      printf(", ");
      cell = cell -> next;
    }
  }
  printf("}\n");
}
