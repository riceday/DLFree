#ifndef __IMPL_MSG_H__
#define __IMPL_MSG_H__

#include <std/list.h>
#include <std/map.h>

enum msg_kind {
  MSG_TYPE_ERR, // just placeholder to avoid using 0
  MSG_TYPE_DATA,
  MSG_TYPE_PING0,
  MSG_TYPE_PING1,
  MSG_TYPE_PONG0,
  MSG_TYPE_PONG1,

  MSG_TYPE_RT,
};

typedef uint64_t sid_t;

typedef struct msg_info{
  int kind;
  int dst_id;
  int src_id;
  int len;
  int remain;

  /* info for data msg chunks */
  sid_t sid;
  int tot_len;
  int seq;
  
} msg_info, *msg_info_t;

void pack_int(void **pp, int i);
int unpack_int(const void **pp);
void pack_buff(void **pp, const void* src, int len);

msg_info_t msg_info_create();
void msg_info_print(msg_info_t minfo);
void msg_info_destroy(msg_info_t minfo);
void msg_info_unpack(msg_info_t minfo, const void** header);
void msg_info_pack(msg_info_t minfo, void **header);

typedef struct msg_buff{
  int len;
  void* data;
  const void* head;
  void* tail;
  
  int using_ext_buff;
  
} msg_buff, *msg_buff_t;

msg_buff_t msg_buff_create(int len);
msg_buff_t msg_buff_create_on_buff(int len, void* buff_to_use);
void msg_buff_destroy(msg_buff_t buff);
const void** msg_buff_head(msg_buff_t buff);
void** msg_buff_tail(msg_buff_t buff);
int msg_buff_len(msg_buff_t buff);
int msg_buff_send_len(msg_buff_t buff);
int msg_buff_recv_len(msg_buff_t buff);

LIST_MAKE_TYPE_INTERFACE(msg_buff);

typedef struct data_msg{
  int len;
  int src_id;
  int recvd;

  int seq;
  double start_time;
  
  msg_buff_list_t chunk_list;
  
  void* user_msg_data;
  
} data_msg, *data_msg_t;

data_msg_t data_msg_create(int len, int src_id, double t);
void* data_msg_destroy(data_msg_t msg);
void* data_msg_buff_tail(data_msg_t msg);
int data_msg_push_chunk(data_msg_t msg, const msg_info_t header, const msg_buff_t chunk);

LIST_MAKE_TYPE_INTERFACE(data_msg);
HASHMAP_MAKE_TYPE_INTERFACE(data_msg);

enum data_msg_status {
  DATA_MSG_MORE,
  DATA_MSG_FULL,
};


#endif // __IMPL_MSG_H__
