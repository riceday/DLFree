#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include "std.h"
#include "vector.h"

/**************/
/*   Vector   */
/**************/

vector_t vector_create(int n){
  vector_t vec = (vector_t) std_malloc(sizeof(vector));
  vec -> n  = n;
  vec -> c  = 0;
  vec -> vec = (void**) std_malloc(sizeof(void*) * n);

  std_pthread_mutex_init(& vec -> m, NULL);

  return vec;
}

void vector_destroy(vector_t vec){
  std_free(vec -> vec);
  std_pthread_mutex_destroy(& vec -> m);
  std_free(vec);
}

vector_t vector_copy(vector_t vec){
  vector_t new_vec = vector_create(vec -> n);
  new_vec -> c = vec -> c;
  memcpy(new_vec -> vec, vec -> vec, sizeof(void*) * vec -> c);
  return new_vec;
}

void vector_add(vector_t vec, void* x){
  std_pthread_mutex_lock(& vec -> m);
  if(vec -> c == vec -> n){ // expand
    vec -> n <<= 2; // double

    // copy
    vec -> vec = (void**) std_realloc(vec -> vec, sizeof(void*) * vec -> n);
  }
  vec -> vec[vec -> c ++] = x;
  std_pthread_mutex_unlock(& vec -> m);
}

void* vector_get(vector_t vec, int i){
  assert(i < vec -> c); // Index out of bounds
  return vec -> vec[i];
}

void* vector_last(vector_t vec){
  void *x;
  std_pthread_mutex_lock(& vec -> m);
  assert(vec -> c > 0); // Index out of bounds
  x = vec -> vec[vec -> c - 1];
  std_pthread_mutex_unlock(& vec -> m);
  
  return x;
}

void* vector_pop(vector_t vec){
  void* x;
  std_pthread_mutex_lock(& vec -> m);
  assert(vec -> c > 0); // empty Vector
  x = vec -> vec[-- vec -> c];
  std_pthread_mutex_unlock(& vec -> m);

  return x;
}

void* vector_del(vector_t vec, int i){
  void* x;
  void** p;
  std_pthread_mutex_lock(& vec -> m);
  assert(i < vec -> c); // Index out of bounds

  p = vec -> vec + i;
  x = *p;
  for(; i < vec -> c; p++, i++){
    *p = *(p+1);
  }
  vec -> c --;
  std_pthread_mutex_unlock(& vec -> m);

  return x;
}

void* vector_remove(vector_t vec, void* x){
  //int i;
  void **p, **q;
  void* y;
  std_pthread_mutex_lock(& vec -> m);

  for(p = vec -> vec, q = p + vec -> c; p < q && *p != x; p++);
  //for(i = 0; i < vec -> c && vec -> vec[i] != x; i++);

  assert(p < q); // element exists in Vector
  y = *p;
  //assert(i < vec -> c); // element exists in Vector
  //y = vec -> vec[i];

  for(; p < q; p++){
    *p = *(p+1);
  }
  //for(;i < vec -> c; i++){
  //vec -> vec[i] = vec -> vec[i+1];
  //}
  vec -> c --;

  std_pthread_mutex_unlock(& vec -> m);

  return y;
}

void vector_clear(vector_t vec){
  std_pthread_mutex_lock(& vec -> m);
  vec -> c = 0;
  std_pthread_mutex_unlock(& vec -> m);
}

void vector_reverse(vector_t vec){
  void **p, **q, *tmp;
  std_pthread_mutex_lock(& vec -> m);
  for(p = vec -> vec, q = vec -> vec + (vec -> c - 1); p < q; p++, q--){
    tmp = *p;
    *p = *q;
    *q = tmp;
  }
  std_pthread_mutex_unlock(& vec -> m);
}

int vector_contains(vector_t vec, void* x){
  void **p, **q;
  int found = 0;
  std_pthread_mutex_lock(& vec -> m);
  for(p = vec -> vec, q = p + vec -> c; p < q; p++){
    if(*p == x){
      found = 1;
      break;
    }
  }
  std_pthread_mutex_unlock(& vec -> m);
  
  return found;
}

int vector_size(vector_t vec){
  return vec -> c;
}

#if !HAVE_QSORT

static void
_swap(void **x, void **y){
  void* tmp = *x;
  *x = *y;
  *y = tmp;
}

static void
_qsort(void** vec, int start, int finish, int(*cmp)(const void*, const void*)){
  int p_idx = start; /* TODO: choose good pivot */
  void *pivot = vec[p_idx];
  int i, j;

  /* partition section */
  _swap(vec + p_idx, vec + finish); /* bring pivot element to back */
  for(i = start, j = finish; i < j; i++){
    if(cmp(vec[i], pivot) <= 0)
      continue;
    
    while(cmp(pivot, vec[j]) <= 0) j--;
    if(i < j){ /* swap is possible */
      _swap(vec + i, vec + j);
      i++;
    }else{
      break;
    }
  }
  /* the pivot becomes the i-th element */
  _swap(vec + finish, vec + i); /* bring pivot element to final pos. */
  
  if(start < i - 1) /* sort before pivot */
    _qsort(vec, start, i - 1, cmp);
  if(i + 1 < finish) /* sort after pivot */
    _qsort(vec, i + 1, finish, cmp);
}

#endif // HAVE_QSORT

void vector_sort(vector_t vec, int(*cmp)(const void*, const void*)){
  std_pthread_mutex_lock(& vec -> m);

#if HAVE_QSORT
  qsort(vec -> vec, vec -> c, sizeof(void*), cmp);
#else
  _qsort(vec -> vec, 0, vec -> c - 1, cmp);
#endif
  
  std_pthread_mutex_unlock(& vec -> m);
}

void vector_shuffle(vector_t vec, int seed){
  int i, j;
  void *tmp;
  
  std_pthread_mutex_lock(& vec -> m);
  srand(seed);
  for(i = 0; i < vec -> c; i++){
    j = rand() % vec -> c;
    tmp = vec -> vec[i];
    vec -> vec[i] = vec -> vec[j];
    vec -> vec[j] = tmp;
  }
  std_pthread_mutex_unlock(& vec -> m);
}

void vector_print(vector_t vec, void(*func)(const void*)){
  void** p, **q;
  printf( "Vector(%d) [", vec -> c);
  for(p = vec -> vec, q = p + vec -> c; p < q; p++){
    func(*p);
    printf(", ");
    fflush(stdout);
  }
  printf("]\n");
}
