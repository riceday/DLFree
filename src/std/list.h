#ifndef __LIST_H__
#define __LIST_H__

typedef struct list_cell list_cell, *list_cell_t;

struct list_cell{
  void* data;
  struct list_cell* next;
  struct list_cell* prev;
};

void* list_cell_data(list_cell_t cell);
list_cell_t list_cell_next(list_cell_t cell);
list_cell_t list_cell_prev(list_cell_t cell);

typedef struct list{
  list_cell_t head;
  int n;
} list, *list_t;

list_t list_create();
void list_destroy(list_t lt);
list_cell_t list_head(list_t lt);
list_cell_t list_tail(list_t lt);
list_cell_t list_end(list_t lt);
void list_append(list_t lt, void* x);
void list_appendleft(list_t lt, void* x);
void list_insert(list_t lt, int i, void* x);
void* list_remove(list_t lt, list_cell_t cell);
void* list_popleft(list_t lt);
void* list_pop(list_t lt);
int list_size(list_t lt);
void list_print(list_t lt, void (*func)(const void*) );
  
#define LIST_MAKE_TYPE_INTERFACE(TYPE) \
typedef list_cell_t TYPE ## _list_cell_t; \
TYPE ## _t TYPE ## _list_cell_data(TYPE ## _list_cell_t cell); \
TYPE ## _list_cell_t TYPE ## _list_cell_next(TYPE ## _list_cell_t cell); \
TYPE ## _list_cell_t TYPE ## _list_cell_prev(TYPE ## _list_cell_t cell); \
typedef list_t TYPE ## _list_t; \
TYPE ## _list_t TYPE ## _list_create(); \
void TYPE ## _list_destroy(TYPE ## _list_t lt); \
TYPE ## _list_cell_t TYPE ## _list_head(TYPE ## _list_t lt); \
TYPE ## _list_cell_t TYPE ## _list_tail(TYPE ## _list_t lt); \
TYPE ## _list_cell_t TYPE ## _list_end(TYPE ## _list_t lt); \
void TYPE ## _list_append(TYPE ## _list_t lt, TYPE ## _t x); \
void TYPE ## _list_appendleft(TYPE ## _list_t lt, TYPE ## _t x); \
void TYPE ## _list_insert(TYPE ## _list_t lt, int i, TYPE ## _t x); \
TYPE ## _t TYPE ## _list_remove(TYPE ## _list_t lt, TYPE ## _list_cell_t x); \
TYPE ## _t TYPE ## _list_popleft(TYPE ## _list_t lt); \
TYPE ## _t TYPE ## _list_pop(TYPE ## _list_t lt); \
int TYPE ## _list_size(TYPE ## _list_t lt); \
void TYPE ## _list_print(TYPE ## _list_t lt, void (*func)(const TYPE ## _t) );
  
#define LIST_MAKE_TYPE_IMPLEMENTATION(TYPE) \
TYPE ## _t TYPE ## _list_cell_data(TYPE ## _list_cell_t cell){ \
  return (TYPE ## _t)list_cell_data((list_cell_t)cell); \
} \
TYPE ## _list_cell_t TYPE ## _list_cell_next(TYPE ## _list_cell_t cell){ \
  return list_cell_next((list_cell_t)cell); \
} \
TYPE ## _list_cell_t TYPE ## _list_cell_prev(TYPE ## _list_cell_t cell){ \
  return list_cell_prev((list_cell_t)cell); \
} \
TYPE ## _list_t TYPE ## _list_create(){ \
  return (TYPE ## _list_t)list_create(); \
} \
void TYPE ## _list_destroy(TYPE ## _list_t lt){ \
  list_destroy((list_t)lt); \
} \
TYPE ## _list_cell_t TYPE ## _list_head(TYPE ## _list_t lt){ \
  return (TYPE ## _list_cell_t)list_head((list_t)lt); \
} \
TYPE ## _list_cell_t TYPE ## _list_tail(TYPE ## _list_t lt){ \
  return (TYPE ## _list_cell_t)list_tail((list_t)lt); \
} \
TYPE ## _list_cell_t TYPE ## _list_end(TYPE ## _list_t lt){ \
  return (TYPE ## _list_cell_t)list_end((list_t)lt); \
} \
void TYPE ## _list_append(TYPE ## _list_t lt, TYPE ## _t x){ \
  list_append((list_t)lt, (void*)x); \
} \
void TYPE ## _list_appendleft(TYPE ## _list_t lt, TYPE ## _t x){ \
  list_appendleft((list_t)lt, (void*)x); \
} \
void TYPE ## _list_insert(TYPE ## _list_t lt, int i, TYPE ## _t x){ \
  list_insert((list_t)lt, i, (void*)x); \
} \
TYPE ## _t TYPE ## _list_remove(TYPE ## _list_t lt, TYPE ## _list_cell_t x){ \
  return (TYPE ## _t)list_remove((list_t)lt, (list_cell_t)x); \
}\
 TYPE ## _t TYPE ## _list_popleft(TYPE ## _list_t lt){ \
  return (TYPE ## _t)list_popleft((list_t)lt); \
} \
TYPE ## _t TYPE ## _list_pop(TYPE ## _list_t lt){ \
  return (TYPE ## _t)list_pop((list_t)lt); \
} \
int TYPE ## _list_size(TYPE ## _list_t lt){ \
  return list_size((list_t)lt); \
} \
void TYPE ## _list_print(TYPE ## _list_t lt, void (*func)(const TYPE ## _t) ){ \
  list_print((list_t)lt, (void(*)(const void*))func); \
}

#endif // __LIST_H__
