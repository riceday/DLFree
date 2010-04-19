#ifndef __IMPL_CHANNEL_H__
#define __IMPL_CHANNEL_H__

#include <std/list.h>
#include <std/map.h>
#include "sock.h"
#include "msg.h"

#define CHANNEL_PEER_UNKNOWN (-1)

#define CHANNEL_MSG_HEADERLEN (100)
#define CHANNEL_MSG_QUEUE_LIMIT (100) // size of the chunk queue associated with each channel for sending

enum channel_connect_status{
  CHANNEL_CONNECT_INPROGRESS,
  CHANNEL_CONNECT_ESTABLISHED,
};

enum channel_status{
  CHANNEL_INIT,
  CHANNEL_SETUP,
  CHANNEL_ACTIVE,
  CHANNEL_BLOCKING,
};

enum channel_activate_status{
  CHANNEL_ACTIVATE_OK,
  CHANNEL_ACTIVATE_ERR,
};

enum channel_read_status{
  CHANNEL_READ_OK,
  CHANNEL_READ_DONE,
  CHANNEL_READ_ERR,
  CHANNEL_READ_EAGAIN,
};

enum channel_write_status{
  CHANNEL_WRITE_OK,
  CHANNEL_WRITE_EAGAIN,
  CHANNEL_WRITE_ERR,
};

enum channel_pipeline_status{
  CHANNEL_PIPELINE_OK,
  CHANNEL_PIPELINE_FAIL,
};

typedef struct channel channel, *channel_t;

LIST_MAKE_TYPE_INTERFACE(channel);
HASHMAP_MAKE_TYPE_INTERFACE(channel);

struct channel{
  sock_t sk;

  /* whether it is established or not */
  int connect_state;
  
  /* id related stuff */
  int peer_id;
  char peer_hostname[20];

  /* msg buffer related stuff */
  void* header_buff;
  int header_off;

  msg_buff_list_t buff_queue;
  
  msg_info_t msg_info;
  msg_buff_t curr_buff;
  
  /* CHANNEL dependencies */
  int state;
  channel_t next;
  channel_t prev;
  channel_list_t wait_queue;

  /* stuff for local pseudo-channel */
  int pipe[2];
  pthread_mutex_t lock;
  pthread_cond_t cond;

  /* stats */
  long rx_count;
  long tx_count;
};

channel_t channel_active_create(sock_t sk);
channel_t channel_setup_create(sock_t sk);
channel_t channel_local_create(int local_pid);

void channel_print(channel_t chan);
void channel_destroy(channel_t chan);
void channel_local_destroy(channel_t chan);

int channel_is_readable(channel_t chan);
int channel_is_writable(channel_t chan);
sock_t channel_get_sock(channel_t chan);
int channel_make_active(channel_t chan);

void channel_make_established(channel_t chan);
int channel_is_established(channel_t chan);

int channel_read_header(channel_t chan, int* msg_kind);

void channel_setup_chunk(channel_t chan);
void channel_setup_chunk_with_buff(channel_t chan, void* buff);
void channel_setup_msg(channel_t chan);

int channel_read_msg(channel_t chan, msg_buff_t *buff);
void channel_local_push_chunk(channel_t chan, msg_info_t minfo, const void *buff, int len);
msg_buff_t channel_local_pop_chunk(channel_t chan);
void channel_local_pop_chunk_ack(channel_t chan);
int channel_read_chunk(channel_t chan, msg_buff_t *buff);

int channel_pipeline_chunk(channel_t chan, channel_t next);

void channel_send_msg(channel_t chan, msg_info_t minfo, const void* buff, int len);
int channel_write(channel_t chan, int *unblock);

#endif // __IMPL_CHANNEL_H__
