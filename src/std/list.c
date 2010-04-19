#include <assert.h>
#include <stdio.h>
#include "std.h"
#include "list.h"

/******************/
/*   List/Deque   */
/******************/


list_cell_t list_cell_create(void* x){
  list_cell_t cell;
  cell = (list_cell_t)std_malloc(sizeof(list_cell));
  cell -> data = x;
  cell -> next = NULL;
  cell -> prev = NULL;
  return cell;
}

list_cell_t list_cell_destroy(list_cell_t cell){
  list_cell_t next = cell -> next;
  cell -> next = NULL;
  std_free(cell);
  return next;
}

void* list_cell_data(list_cell_t cell){
  return cell -> data;
}

list_cell_t list_cell_next(list_cell_t cell){
  return cell -> next;
}

list_cell_t list_cell_prev(list_cell_t cell){
  return cell -> prev;
}

list_t list_create(){
  list_t lt = (list_t)std_malloc(sizeof(list));
  list_cell_t dummy = list_cell_create(NULL); // dummy cell

  // list loops around
  dummy -> next = dummy;
  dummy -> prev = dummy;
  
  lt -> head = dummy;
  lt -> n = 0;

  return lt;
}

void list_destroy(list_t lt){
  list_cell_t cell = lt -> head;

  //destroy all cells
  do{
    cell = list_cell_destroy(cell);
  } while(cell != lt -> head);
  
  std_free(lt);
}

list_cell_t list_head(list_t lt){
  return lt -> head -> next;
}

list_cell_t list_tail(list_t lt){
  return lt -> head -> prev;
}

list_cell_t list_end(list_t lt){
  return lt -> head;
}

void list_append(list_t lt, void* x){
  list_cell_t cell;
  cell = list_cell_create(x);


  cell -> prev = lt -> head -> prev; // previous tail
  cell -> next = lt -> head;
  
  lt -> head -> prev -> next = cell; // update tail next
  lt -> head -> prev = cell; // update head prev
  
  lt -> n ++;
}

void list_appendleft(list_t lt, void* x){
  list_cell_t cell;
  cell = list_cell_create(x);

  cell -> next = lt -> head -> next;
  cell -> prev = lt -> head;
  
  lt -> head -> next -> prev = cell; // update old head's prev
  lt -> head -> next = cell; // update head next

  lt -> n ++;
}

void list_insert(list_t lt, int i, void* x){
  int j;
  list_cell_t cell, next, prev, new;

  new = list_cell_create(x);

  cell = lt -> head;

  for(j = 0; j < i; j++){
    cell = cell -> next;
    assert(cell != lt -> head); // enough elements exists in List
  }

  next = cell -> next;
  prev = cell;

  //update new
  new -> next = next;
  new -> prev = prev;

  prev -> next = new; // update prev
  next -> prev = new; // update next

  lt -> n ++;
}

void* list_remove(list_t lt, list_cell_t cell){
  void* x;
  list_cell_t prev, next;
  prev = cell -> prev;
  next = cell -> next;

  //asert cell is part of this list
  assert(prev != NULL && next != NULL);
  assert(prev -> next -> next == next);

  //assert not head
  assert(cell != lt -> head);
  //assert non-empty List
  assert(lt -> n > 0);

  prev -> next = next;
  next -> prev = prev;
  lt -> n --;

  x = cell -> data;
  list_cell_destroy(cell);
  return x;
}
		 
void* list_popleft(list_t lt){
  list_cell_t head;
  void* x;

  head = lt -> head -> next;
  assert(head != lt -> head); // empty List

  head -> next -> prev = lt -> head; // update new head
  lt -> head -> next = head -> next; // update head
  
  lt -> n --;

  x = head -> data;
  list_cell_destroy(head);
  return x;
}

void* list_pop(list_t lt){
  list_cell_t tail;
  void* x;

  tail = lt -> head -> prev;
  assert(tail != lt -> head); // empty List

  lt -> head -> prev = tail -> prev; // update tail
  tail -> prev -> next = lt -> head; // update new tail

  lt -> n --;

  x = tail -> data;
  list_cell_destroy(tail);
  return x;
}

int list_size(list_t lt){
  return lt -> n;
}

void list_print(list_t lt, void (*func)(const void*) ){
  list_cell_t cell;
  
  printf("List(%d) [", lt -> n);
  for(cell = lt -> head -> next; cell != lt -> head; cell = list_cell_next(cell)){
    func(list_cell_data(cell));
    printf(", ");
  }
  printf("]\n");
}

