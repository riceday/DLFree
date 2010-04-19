#ifndef __VECTOR_H__
#define __VECTOR_H__

#include <pthread.h>

typedef struct vector{
  int n;
  int c;
  void** vec;
  pthread_mutex_t m;
} vector, * vector_t;

vector_t vector_create(int n);
void vector_destroy(vector_t vec);
vector_t vector_copy(vector_t vec);
void vector_add(vector_t vec, void* x);
void* vector_get(vector_t vec, int i);
void* vector_last(vector_t vec);
void* vector_pop(vector_t vec);
void* vector_del(vector_t vec, int i);
void* vector_remove(vector_t vec, void* x);
void vector_clear(vector_t vec);
void vector_reverse(vector_t vec);
int vector_contains(vector_t vec, void* x);
int vector_size(vector_t vec);
void vector_sort(vector_t vec, int(*cmp)(const void*, const void*));
void vector_shuffle(vector_t vec, int seed);
void vector_print(vector_t vec, void(*func)(const void*));

#define VECTOR_MAKE_TYPE_INTERFACE(TYPE) \
typedef vector_t TYPE ## _vector_t;  \
TYPE ## _vector_t TYPE ## _vector_create(int n); \
void TYPE ## _vector_destroy(TYPE ## _vector_t vec); \
TYPE ## _vector_t TYPE ## _vector_copy(TYPE ## _vector_t vec); \
void TYPE ## _vector_add(TYPE ## _vector_t vec, TYPE ## _t x); \
TYPE ## _t TYPE ## _vector_get(TYPE ## _vector_t vec, int i); \
TYPE ## _t TYPE ## _vector_last(TYPE ## _vector_t vec); \
TYPE ## _t TYPE ## _vector_pop(TYPE ## _vector_t vec); \
TYPE ## _t TYPE ## _vector_del(TYPE ## _vector_t vec, int i); \
TYPE ## _t TYPE ## _vector_remove(TYPE ## _vector_t vec, TYPE ## _t x); \
void TYPE ## _vector_clear(TYPE ## _vector_t vec); \
void TYPE ## _vector_reverse(TYPE ## _vector_t vec); \
int TYPE ## _vector_contains(TYPE ## _vector_t vec, TYPE ## _t x); \
int TYPE ## _vector_size(TYPE ## _vector_t vec); \
void TYPE ## _vector_sort(TYPE ## _vector_t vec, int(*cmp)(const TYPE ## _t, const TYPE ## _t)); \
void TYPE ## _vector_shuffle(TYPE ## _vector_t vec, int seed); \
void TYPE ## _vector_print(TYPE ## _vector_t vec, void (*func)(const TYPE ## _t));


#define VECTOR_MAKE_TYPE_IMPLEMENTATION(TYPE) \
TYPE ## _vector_t TYPE ## _vector_create(int n){ \
  vector_t vec = vector_create(n); \
  return (TYPE ## _vector_t)vec; \
} \
void TYPE ## _vector_destroy(TYPE ## _vector_t vec){ \
  vector_destroy((vector_t)vec ); \
} \
TYPE ## _vector_t TYPE ## _vector_copy(TYPE ## _vector_t vec){ \
  return (TYPE ## _vector_t)vector_copy((vector_t)vec); \
} \
void TYPE ## _vector_add(TYPE ## _vector_t vec, TYPE ## _t x){ \
  vector_add((vector_t)vec, (void*) x); \
} \
TYPE ## _t TYPE ## _vector_get(TYPE ## _vector_t vec, int i){ \
  return (TYPE ## _t)vector_get((vector_t)vec, i); \
} \
TYPE ## _t TYPE ## _vector_last(TYPE ## _vector_t vec){ \
  return (TYPE ## _t)vector_last((vector_t)vec); \
} \
TYPE ## _t TYPE ## _vector_pop(TYPE ## _vector_t vec){ \
  void* x = vector_pop((vector_t)vec); \
  return (TYPE ## _t)x; \
} \
TYPE ## _t TYPE ## _vector_del(TYPE ## _vector_t vec, int i){ \
  void* x = vector_del((vector_t)vec, i); \
  return (TYPE ## _t)x; \
} \
TYPE ## _t TYPE ## _vector_remove(TYPE ## _vector_t vec, TYPE ## _t x){ \
  return (TYPE ## _t) vector_remove((vector_t)vec, (void*) x); \
} \
void TYPE ## _vector_clear(TYPE ## _vector_t vec){ \
  vector_clear((vector_t)vec ); \
} \
void TYPE ## _vector_reverse(TYPE ## _vector_t vec){ \
  vector_reverse((vector_t)vec ); \
} \
int TYPE ## _vector_contains(TYPE ## _vector_t vec, TYPE ## _t x){ \
  return vector_contains((vector_t)vec, (void*) x); \
} \
int TYPE ## _vector_size(TYPE ## _vector_t vec){ \
  return vector_size((vector_t)vec); \
} \
void TYPE ## _vector_sort(TYPE ## _vector_t vec, int(*cmp)(const TYPE ## _t, const TYPE ## _t)){ \
  int _cmp_(const void* x, const void* y){ \
    return cmp(*(const TYPE ## _t*)x, *(const TYPE ## _t*)y); \
  } \
  vector_sort((vector_t)vec, _cmp_); \
} \
void TYPE ## _vector_shuffle(TYPE ## _vector_t vec, int seed){ \
  return vector_shuffle((vector_t)vec, seed); \
} \
void TYPE ## _vector_print(TYPE ## _vector_t vec, void (*func)(const TYPE ## _t)){ \
  vector_print((vector_t)vec, (void(*)(const void*))func); \
}

#endif // __VECTOR_H__
