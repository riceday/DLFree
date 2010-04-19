#include <assert.h>

#include <std/std.h>
#include <std/bytes.h>
#include "rtable.h"

VECTOR_MAKE_TYPE_IMPLEMENTATION(overlay_rtable)

overlay_rtable_entry_t
overlay_rtable_entry_create(int dst_pid, int *path, int hops, int metric, int width){
  overlay_rtable_entry_t entry = (overlay_rtable_entry_t) std_malloc(sizeof(overlay_rtable_entry));

  entry -> dst_pid = dst_pid;
  entry -> hops = hops;
  entry -> metric = metric;
  entry -> width = width;
  entry -> path = (int*)std_malloc(sizeof(int) * (hops + 1));
  std_memcpy(entry -> path, path, sizeof(int) * (hops + 1));
  
  return entry;
}

void
overlay_rtable_entry_destroy(overlay_rtable_entry_t entry){
  std_free(entry -> path);
  std_free(entry);
}

void
overlay_rtable_entry_pack(overlay_rtable_entry_t entry, void**pp){
  int i;
  pack_int(pp, entry -> dst_pid);
  pack_int(pp, entry -> hops);
  pack_int(pp, entry -> metric);
  pack_int(pp, entry -> width);
  for(i = 0; i < entry -> hops + 1; i++)
    pack_int(pp, entry -> path[i]);
}

int
overlay_rtable_entry_pack_len(overlay_rtable_entry_t entry){
  int c = 0;
  c += sizeof(int); /* dstpid */
  c += sizeof(int); /* hops */
  c += sizeof(int); /* metric */
  c += sizeof(int); /* width */
  c += (sizeof(int) * (entry -> hops + 1)); /* path */
  return c;
}

overlay_rtable_entry_t
overlay_rtable_entry_create_by_unpack(const void**pp){
  overlay_rtable_entry_t entry;
  int i, dst_pid, metric, width, hops, *path;
  dst_pid = unpack_int(pp);
  hops = unpack_int(pp);
  metric = unpack_int(pp);
  width = unpack_int(pp);
  path = (int*)std_malloc(sizeof(int) * (hops + 1));
  for(i = 0; i < hops + 1; i++)
    path[i] = unpack_int(pp);
  entry = overlay_rtable_entry_create(dst_pid, path, hops, metric, width);
  std_free(path);
  return entry;
}

void
overlay_rtable_entry_print(overlay_rtable_entry_t entry){
  int i;
  printf("entry(%d: ", entry -> dst_pid);
  printf("%d %d [", entry -> metric, entry -> width);
  for(i = 0; i < entry -> hops + 1; i++){
    printf("%d, ", entry -> path[i]);
  }
  printf("])");
}

void
overlay_rtable_entry_print_with_name(overlay_rtable_entry_t entry, const char** hostnames){
  int i;
  printf("entry(%d: ", entry -> dst_pid);
  printf("%d %d [", entry -> metric, entry -> width);
  for(i = 0; i < entry -> hops + 1; i++){
    printf("%s, ", hostnames[entry -> path[i]]);
  }
  printf("])");
}

overlay_rtable_t
overlay_rtable_create(int srcpid, int nentry){
  overlay_rtable_t rt = (overlay_rtable_t) std_malloc(sizeof(overlay_rtable));
  int pid;
  rt -> srcpid = srcpid;
  rt -> nentry = nentry;
  rt -> entries = (overlay_rtable_entry_t*) std_malloc(sizeof(overlay_rtable_entry_t) * nentry);
  for(pid = 0; pid < nentry; pid++){
    rt -> entries[pid] = NULL;
  }
  return rt;
}

void
overlay_rtable_destroy(overlay_rtable_t rt){
  int pid;
  overlay_rtable_entry_t entry;
  for(pid = 0; pid < rt -> nentry; pid++){
    entry = rt -> entries[pid];
    if(entry != NULL){
      overlay_rtable_entry_destroy(entry);
    }
  }
  std_free(rt -> entries);
  std_free(rt);
}

int
overlay_rtable_size(overlay_rtable_t rt){
  return rt -> nentry;
}

void
overlay_rtable_add_entry(overlay_rtable_t rt, overlay_rtable_entry_t entry){
  assert(rt -> entries[entry -> dst_pid] == NULL);
  rt -> entries[entry -> dst_pid] = entry;
}

overlay_rtable_entry_t
overlay_rtable_get_entry(overlay_rtable_t rt, int pid){
  return rt -> entries[pid];
}

void
overlay_rtable_pack(overlay_rtable_t rt, void**pp){
  int pid;
  pack_int(pp, rt -> srcpid);
  pack_int(pp, rt -> nentry);
  for(pid = 0; pid < rt -> nentry; pid++){
    if(pid == rt -> srcpid)
      continue;
    assert(rt -> entries[pid] != NULL);
    overlay_rtable_entry_pack(rt -> entries[pid], pp);
  }
}

int
overlay_rtable_pack_len(overlay_rtable_t rt){
  int c = 0, pid;
  c += sizeof(int); /* srcpid */
  c += sizeof(int); /* nentry */
  for(pid = 0; pid < rt -> nentry; pid++){
    if(pid == rt -> srcpid)
      continue;
    assert(rt -> entries[pid] != NULL);
    c += overlay_rtable_entry_pack_len(rt -> entries[pid]);
  }
  return c;
}

overlay_rtable_t
overlay_rtable_create_by_unpack(const void**pp){
  int pid, srcpid, nentry;
  overlay_rtable_t rt;
  overlay_rtable_entry_t entry;
  srcpid = unpack_int(pp);
  nentry = unpack_int(pp);
  rt = overlay_rtable_create(srcpid, nentry);
  for(pid = 0; pid < nentry; pid++){
    if(pid == srcpid)
      continue;
    entry = overlay_rtable_entry_create_by_unpack(pp);
    overlay_rtable_add_entry(rt, entry);
  }
  return rt;
}

void
overlay_rtable_print(overlay_rtable_t rt){
  int pid;
  printf("RT(%d) BEGIN\n", rt -> srcpid);
  for(pid = 0; pid < rt -> nentry; pid++){
    if(pid == rt -> srcpid)
      continue;
    overlay_rtable_entry_print(rt -> entries[pid]);
    printf("\n");
  }
  printf("RT(%d) END\n", rt -> srcpid);
}

void
overlay_rtable_print_with_name(overlay_rtable_t rt, const char** hostnames){
  int pid;
  printf("RT(%d) BEGIN\n", rt -> srcpid);
  for(pid = 0; pid < rt -> nentry; pid++){
    if(pid == rt -> srcpid)
      continue;
    overlay_rtable_entry_print_with_name(rt -> entries[pid], hostnames);
    printf("\n");
  }
  printf("RT(%d) END\n", rt -> srcpid);
}
