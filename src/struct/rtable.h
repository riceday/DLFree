#ifndef __GRAPH_STRUCT_H__
#define __GRAPH_STRUCT_H__

#include <std/std.h>
#include <std/vector.h>

typedef struct overlay_rtable_entry {
  int dst_pid;
  int* path;
  int hops;
  int metric;
  int width;
} overlay_rtable_entry, *overlay_rtable_entry_t;

typedef struct overlay_rtable {
  int srcpid;
  int nentry;
  overlay_rtable_entry_t* entries;
} overlay_rtable, *overlay_rtable_t;

VECTOR_MAKE_TYPE_INTERFACE(overlay_rtable)

overlay_rtable_entry_t overlay_rtable_entry_create(int dst_pid, int *path, int hops, int metric, int width);
void overlay_rtable_entry_print_with_name(overlay_rtable_entry_t entry, const char** hostnames);

overlay_rtable_t overlay_rtable_create(int srcpid, int nentry);
void overlay_rtable_destroy(overlay_rtable_t rt);
int overlay_rtable_size(overlay_rtable_t rt);
void overlay_rtable_add_entry(overlay_rtable_t rt, overlay_rtable_entry_t entry);
overlay_rtable_entry_t overlay_rtable_get_entry(overlay_rtable_t rt, int pid);
void overlay_rtable_print(overlay_rtable_t rt);
void overlay_rtable_print_with_name(overlay_rtable_t rt, const char** hostnames);
void overlay_rtable_pack(overlay_rtable_t rt, void**pp);
int overlay_rtable_pack_len(overlay_rtable_t rt);
overlay_rtable_t overlay_rtable_create_by_unpack(const void**pp);

#endif // __GRAPH_STRUCT_H__
