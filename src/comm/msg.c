#include <string.h>
#include <stdio.h>
#include <assert.h>

#include <std/std.h>
#include <std/list.h>
#include <std/bytes.h>
#include "impl/msg.h"

LIST_MAKE_TYPE_IMPLEMENTATION(msg_buff);

LIST_MAKE_TYPE_IMPLEMENTATION(data_msg);
HASHMAP_MAKE_TYPE_IMPLEMENTATION(data_msg);

msg_info_t
msg_info_create(){
  msg_info_t minfo = (msg_info_t)std_malloc(sizeof(msg_info));
  minfo -> kind = -1;
  minfo -> dst_id = -1;
  minfo -> src_id = -1;
  minfo -> len = -1;
  minfo -> remain = 0;

  /* info for data msg chunks */
  minfo -> sid = -1;
  minfo -> tot_len = -1;
  minfo -> seq = -1;

  return minfo;
}

void
msg_info_print(msg_info_t minfo){
  printf("minfo(kind:%d, dst:%d, src:%d, len:%d, remain:%d sid:%llu tot:%d seq:%d)\n",
	 minfo -> kind,
	 minfo -> dst_id,
	 minfo -> src_id,
	 minfo -> len,
	 minfo -> remain,
	 minfo -> sid,
	 minfo -> tot_len,
	 minfo -> seq);
}

void
msg_info_destroy(msg_info_t minfo){
  std_free(minfo);
}

void
msg_info_unpack(msg_info_t minfo, const void** header){
  const void **p = header;
  minfo -> kind = unpack_int(p);
  minfo -> dst_id = unpack_int(p);
  minfo -> src_id = unpack_int(p);
  minfo -> len = unpack_int(p);
  minfo -> sid = unpack_uint64(p);
  minfo -> tot_len = unpack_int(p);
  minfo -> seq = unpack_int(p);
  minfo -> remain = minfo -> len;
}

void
msg_info_pack(msg_info_t minfo, void **header){
  void **p = header;

  pack_int(p, minfo -> kind);
  pack_int(p, minfo -> dst_id);
  pack_int(p, minfo -> src_id);
  pack_int(p, minfo -> len);
  pack_uint64(p, minfo -> sid);
  pack_int(p, minfo -> tot_len);
  pack_int(p, minfo -> seq);
}

/* msg_buff_t */
/* msg_buff_create(int len){ */
/*   msg_buff_t buff = (msg_buff_t)std_malloc(sizeof(msg_buff)); */
/*   buff -> data = std_malloc(len); */
/*   buff -> head = buff -> data; */
/*   buff -> tail = buff -> data; */
/*   buff -> len = len; */
/*   return buff; */
/* } */

msg_buff_t
msg_buff_create_on_buff(int len, void* buff_to_use){
  msg_buff_t buff = (msg_buff_t)std_malloc(sizeof(msg_buff));

/*   if buffer is supplied, write on that */
/*   if not allocate new chunk */
	   
  if(buff_to_use != NULL){
    buff -> data = buff_to_use;
    buff -> using_ext_buff = 1;
  }else{
    buff -> data = std_malloc(len);
    buff -> using_ext_buff = 0;
  }
  
  buff -> head = buff -> data;
  buff -> tail = buff -> data;
  buff -> len = len;
  
  return buff;
}

msg_buff_t
msg_buff_create(int len){
  return msg_buff_create_on_buff(len, NULL);
}

void
msg_buff_destroy(msg_buff_t buff){
  /* only free buffer if internally allocated */
  if(buff -> using_ext_buff == 0)
    std_free(buff -> data);
  
  std_free(buff);
}

const void**
msg_buff_head(msg_buff_t buff){
  return &(buff -> head);
}

void**
msg_buff_tail(msg_buff_t buff){
  return &(buff -> tail);
}

int
msg_buff_len(msg_buff_t buff){
  return buff -> len;
}

int
msg_buff_send_len(msg_buff_t buff){
  return (buff -> tail - buff -> head);
}

int
msg_buff_recv_len(msg_buff_t buff){
  return (buff -> data + buff -> len) - (buff -> tail);
}


data_msg_t
data_msg_create(int len, int src_id, double t){
  data_msg_t msg = (data_msg_t) std_malloc(sizeof(data_msg));

  msg -> len = len;
  msg -> src_id = src_id;
  msg -> recvd = 0;
  msg -> chunk_list = msg_buff_list_create();
  msg -> user_msg_data = (void*) std_malloc(len);

  msg -> seq = -1;
  msg -> start_time = t;

  return msg;
}

void*
data_msg_destroy(data_msg_t msg){
  msg_buff_t chunk;
  void* data = msg -> user_msg_data;
  while(msg_buff_list_size(msg -> chunk_list)){
    chunk = msg_buff_list_popleft(msg -> chunk_list);
    msg_buff_destroy(chunk);
  }
  msg_buff_list_destroy(msg -> chunk_list);

  std_free(msg);
  return data;
}

void*
data_msg_buff_tail(data_msg_t msg){
  return (msg -> user_msg_data + msg -> recvd);
}

int
data_msg_push_chunk(data_msg_t msg, const msg_info_t header, const msg_buff_t chunk){
  int dseq;
  assert(header -> src_id == msg -> src_id);

  /* assert that seq is same or is only 1 apart */
  dseq = header -> seq - msg -> seq;
  assert(dseq == 0 || dseq == 1);

  msg -> seq = header -> seq;
  
  msg_buff_list_append(msg -> chunk_list, chunk);

  msg -> recvd += msg_buff_len(chunk);
  
  //printf("data_msg_push_chunk: recvd %d/%d \n", msg -> recvd, msg -> len);fflush(stdout);
  assert(msg -> recvd <= msg -> len);

  if(msg -> recvd == msg -> len)
    return DATA_MSG_FULL;
  else
    return DATA_MSG_MORE;
}
