#include <assert.h>
#include <string.h>

#include <std/std.h>
#include <std/list.h>
#include <std/map.h>
#include <iface/iface.h>
#include "impl/msg.h"
#include "impl/channel.h"

LIST_MAKE_TYPE_IMPLEMENTATION(channel);
HASHMAP_MAKE_TYPE_IMPLEMENTATION(channel);

static channel_t
base_channel_create(sock_t sk){
  channel_t chan = std_malloc(sizeof(channel));
  
  chan -> sk = sk;
  chan -> peer_id = CHANNEL_PEER_UNKNOWN;
  chan -> connect_state = CHANNEL_CONNECT_ESTABLISHED;
  //memset(chan -> peer_hostname, 0, strlen(chan -> peer_hostname));

  chan -> header_buff = std_malloc(CHANNEL_MSG_HEADERLEN);
  chan -> header_off = 0;
  chan -> msg_info = msg_info_create();
  chan -> buff_queue = msg_buff_list_create();
  chan -> curr_buff = NULL;

  chan -> state = CHANNEL_INIT;
  chan -> next = NULL;
  chan -> prev = NULL;
  chan -> wait_queue = channel_list_create();

  chan -> rx_count = 0;
  chan -> tx_count = 0;

  return chan;
}

channel_t
channel_active_create(sock_t sk){
  channel_t chan = base_channel_create(sk);
  chan -> state = CHANNEL_ACTIVE;
  return chan;
}

channel_t
channel_setup_create(sock_t sk){
  channel_t chan = base_channel_create(sk);
  chan -> state = CHANNEL_SETUP;
  chan -> connect_state = CHANNEL_CONNECT_INPROGRESS;
  return chan;
}

channel_t
channel_local_create(int local_pid){
  int pipe[2];
  channel_t chan;
  sock_t fake_sock;
  inet_iface_t local_iface = inet_iface_create_by_str("0.0.0.0"); /* fake iface */

  std_pipe(pipe);
  fake_sock = base_sock_create(pipe[0], local_iface, 0); /* fake sock, use pipe to check readability */
  chan = base_channel_create(fake_sock);
  chan -> state = CHANNEL_ACTIVE;
  chan -> peer_id = local_pid;
  
  pthread_mutex_init(&chan -> lock, NULL);
  pthread_cond_init(&chan -> cond, NULL);

  chan -> pipe[0] = pipe[0];
  chan -> pipe[1] = pipe[1];

  return chan;
}

void
channel_print(channel_t chan){
  inet_iface_t iface = sock_iface(chan -> sk);
  printf("chan(iface: %s state: %d)", inet_iface_in_addr_str(iface), chan -> state);
}

void
channel_destroy(channel_t chan){
  /* destroy all pending messages */
  while(msg_buff_list_size(chan -> buff_queue))
    msg_buff_destroy(msg_buff_list_popleft(chan -> buff_queue));
  msg_buff_list_destroy(chan -> buff_queue);

  std_free(chan -> header_buff);
  msg_info_destroy(chan -> msg_info);
  /* if currently was holding buffer, destroy */
  if(chan -> curr_buff != NULL)
    msg_buff_destroy(chan -> curr_buff);

  channel_list_destroy(chan -> wait_queue);

  sock_destroy(chan -> sk);
  
  std_free(chan);
}

void
channel_local_destroy(channel_t chan){
  /* chan -> pipe[0] will be closed by sock_destroy() */
  std_close(chan -> pipe[1]);

  pthread_mutex_destroy(&chan -> lock);
  pthread_cond_destroy(&chan -> cond);
  
  channel_destroy(chan);
}

int
channel_is_readable(channel_t chan){
  return chan -> state == CHANNEL_ACTIVE;
}

int
channel_is_writable(channel_t chan){
  return msg_buff_list_size(chan -> buff_queue) > 0
    || chan -> state == CHANNEL_SETUP;
}

sock_t
channel_get_sock(channel_t chan){
  return chan -> sk;
}

int
channel_make_active(channel_t chan){
  assert(chan -> state == CHANNEL_SETUP);

  if(sock_connect_status(chan -> sk) != 0)
    return CHANNEL_ACTIVATE_ERR;
  
  chan -> state = CHANNEL_ACTIVE;
  return CHANNEL_ACTIVATE_OK;
}

void
channel_make_established(channel_t chan){
  assert(chan -> connect_state == CHANNEL_CONNECT_INPROGRESS);
  chan -> connect_state = CHANNEL_CONNECT_ESTABLISHED;
}

int
channel_is_established(channel_t chan){
  return chan -> connect_state == CHANNEL_CONNECT_ESTABLISHED;
}

/* pass on read chunk to next channel: */
/* if the send buffer is NOT full, push the chunk onto it */
/* if the send buffer is full, join the wait-queue block until current chunk can be pushed */
int
channel_pipeline_chunk(channel_t chan, channel_t next){
  assert(chan -> curr_buff != NULL);
  if(msg_buff_list_size(next -> buff_queue) < CHANNEL_MSG_QUEUE_LIMIT){
    chan -> state = CHANNEL_ACTIVE;
    msg_buff_list_append(next -> buff_queue, chan -> curr_buff);
    chan -> curr_buff = NULL;
    /* printf("PIPELINE %d ---> %d (dst: %d) size: %d\n", chan -> peer_id, next -> peer_id, chan -> msg_info -> dst_id, msg_buff_list_size(next -> buff_queue));fflush(stdout); */
    return CHANNEL_PIPELINE_OK;
  }else{
    chan -> state = CHANNEL_BLOCKING;
    channel_list_append(next -> wait_queue, chan);
    /* printf("BLOCKED %d -X-> %d (dst: %d)\n", chan -> peer_id, next -> peer_id, chan -> msg_info -> dst_id);fflush(stdout); */
    return CHANNEL_PIPELINE_FAIL;
  }
}

int
channel_read_header(channel_t chan, int* msg_kind){
  int stat;
  int n;
  int off = chan -> header_off;
  const void* p;

  /* read header non-blockingly */
  stat = sock_try_recv_n(chan -> sk, chan -> header_buff + off, CHANNEL_MSG_HEADERLEN - off, &n);
  switch(stat){
  case SOCK_RECV_ERR:
    fprintf(stderr, "channel_read_header: ERROR WHILE READING HEADER\n");
    return CHANNEL_READ_ERR;
  case SOCK_RECV_EOF:
    return CHANNEL_READ_ERR;
  case SOCK_RECV_EAGAIN:
    return CHANNEL_READ_EAGAIN;
  default:
    chan -> header_off += n;
  }
  /* if header is fully received */
  if(chan -> header_off == CHANNEL_MSG_HEADERLEN)
    chan -> header_off = 0;
  else
    return CHANNEL_READ_EAGAIN;
  
  /* unpack header contents */
  p = chan -> header_buff;
  msg_info_unpack(chan -> msg_info, &p);

  /* retrieve msg info type */
  *msg_kind = chan -> msg_info -> kind;

  /* if this fails, likely that header was invalid */
  assert(*msg_kind != MSG_TYPE_ERR);

  return CHANNEL_READ_OK;
}

static void
_channel_setup_chunk(channel_t chan, int size, void* buff){
  assert(chan -> curr_buff == NULL);
  chan -> curr_buff = msg_buff_create_on_buff(size, buff);
}

void
channel_setup_chunk(channel_t chan){
  void** tail;

  int size = chan -> msg_info -> len + CHANNEL_MSG_HEADERLEN;

  assert(chan -> msg_info -> len > 0);
  _channel_setup_chunk(chan, size, NULL);

  /* copy header contents into buffer head */
  tail = msg_buff_tail(chan -> curr_buff);
  pack_buff(tail, chan -> header_buff, CHANNEL_MSG_HEADERLEN);
}

void
channel_setup_chunk_with_buff(channel_t chan, void* buff){
  int size = chan -> msg_info -> len;
  assert(size > 0);
  _channel_setup_chunk(chan, size, buff);
}

void
channel_setup_msg(channel_t chan){
  /* allocate enough memory for msg */
  int size = chan -> msg_info -> len;
  _channel_setup_chunk(chan, size, NULL);
}

int
channel_read_to_buff(channel_t chan, msg_buff_t buff, int* len){
  int stat;
  int n = 0;
  void** tail = msg_buff_tail(buff);

  /* read as much as we can */
  while(*len){
    stat = sock_try_recv_n(chan -> sk, *tail, *len, &n);

    switch(stat){
    case SOCK_RECV_EAGAIN:
      return CHANNEL_READ_OK;
    case SOCK_RECV_EOF:
      //fprintf(stderr, "channel_read_to_buff: EOF WHILE READING TO BUFF\n");
      return CHANNEL_READ_ERR;
    case SOCK_RECV_ERR:
      fprintf(stderr, "channel_read_to_buff: ERROR WHILE READING TO BUFF\n");
      return CHANNEL_READ_ERR;
    }

    *len -= n;
    *tail += n;

    /* for stats */
    chan -> rx_count += n;

    /* what remains to be read gets smaller */
    chan -> msg_info -> remain -= n;
    assert(chan -> msg_info -> remain >= 0);
  }
  return CHANNEL_READ_OK;
}

int
channel_read_msg(channel_t chan, msg_buff_t *buff){
  int remain = chan -> msg_info -> remain;
  int stat = channel_read_to_buff(chan, chan -> curr_buff, &remain);

  /* if buff is full */
  if(remain == 0){
    *buff = chan -> curr_buff;
    chan -> curr_buff = NULL;
    
    return CHANNEL_READ_DONE;
  }

  /* CHANNEL_READ_{ERR, OK} */
  return stat;
}

int
channel_read_chunk(channel_t chan, msg_buff_t *buff){
  int stat;
  int remain;

  assert(chan -> curr_buff != NULL);
  
  remain = msg_buff_recv_len(chan -> curr_buff);

  /* printf("bytes to read: %d\n", remain);fflush(stdout); */
  assert(remain <= chan -> msg_info -> remain);

  stat = channel_read_to_buff(chan, chan -> curr_buff, &remain);

  /* if chunk is full, pass it on to next channel to be sent */
  /* or if the message was fully received */
  if(remain == 0){
    *buff = chan -> curr_buff;
    /* chan -> curr_buff = NULL; */
    /* printf("reading chunk done\n");fflush(stdout); */
    return CHANNEL_READ_DONE;
  }
  /* CHANNEL_READ_{ERR, OK} */
  else
    return stat;
}

msg_buff_t
channel_pack_buff(msg_info_t minfo, const void *buff, int len){
  msg_buff_t msg = msg_buff_create(CHANNEL_MSG_HEADERLEN + len);
  void** head = msg_buff_tail(msg);
  void* body  = *head + CHANNEL_MSG_HEADERLEN; /* where the body starts */

  /* pack header */
  memset(*head, 0, CHANNEL_MSG_HEADERLEN);
  msg_info_pack(minfo, head);
  assert(*head <= body); /* prevent header over-run */

  /* pack body */
  std_memcpy(body, buff, len);

  /* move written data pointer to end */
  *head = body + len;

  return msg;
}

void
channel_send_msg(channel_t chan, msg_info_t minfo, const void* buff, int len){
  msg_buff_t msg = channel_pack_buff(minfo, buff, len);

  /* queue in outgoing msg queue */
  msg_buff_list_append(chan -> buff_queue, msg);
}

int
channel_write(channel_t chan, int *unblock){
  channel_t waiter;
  msg_buff_t buff;
  const void** head;
  int stat;
  int n, len;

  *unblock = 0;
  while(msg_buff_list_size(chan -> buff_queue)){

    /* pop a msg chunk and try to send */
    buff = msg_buff_list_cell_data(msg_buff_list_head(chan -> buff_queue));
    head = msg_buff_head(buff);
    len = msg_buff_send_len(buff);

    while(len){
      stat = sock_try_send_n(chan -> sk, *head, len, &n);
      switch(stat){
      case SOCK_SEND_EAGAIN:
	return CHANNEL_WRITE_EAGAIN;
      case SOCK_SEND_ERR:
	fprintf(stderr, "channel_write: ERROR WHILE WRITING\n");
	return CHANNEL_WRITE_ERR;
      }
      
      len -= n;
      *head += n;

      /* for stats */
      chan -> tx_count += n;
    }

    /* if msg buff completely sent delete */
    msg_buff_list_popleft(chan -> buff_queue);
    msg_buff_destroy(buff);

    /* unblock producer */
    while(channel_list_size(chan -> wait_queue) &&
	  msg_buff_list_size(chan -> buff_queue) < CHANNEL_MSG_QUEUE_LIMIT){
      /* as much as i would like to, this does not hold for send_msg() */
      /* assert(msg_buff_list_size(chan -> buff_queue) < CHANNEL_MSG_QUEUE_LIMIT); */
      waiter = channel_list_popleft(chan -> wait_queue);
      stat = channel_pipeline_chunk(waiter, chan);
      assert(stat == CHANNEL_PIPELINE_OK);
      *unblock = 1;
    }
  }

  return CHANNEL_WRITE_OK;
}

void
channel_local_push_chunk(channel_t chan, msg_info_t minfo, const void *buff, int len){
  const int byte = 1;
  msg_buff_t chunk = channel_pack_buff(minfo, buff, len);

  /* write to blocking finite queue */
  pthread_mutex_lock(&chan -> lock);
  while(msg_buff_list_size(chan -> buff_queue) >= CHANNEL_MSG_QUEUE_LIMIT)
    pthread_cond_wait(&chan -> cond, &chan -> lock);
  
  msg_buff_list_append(chan -> buff_queue, chunk);
  pthread_mutex_unlock(&chan -> lock);

  std_write(chan -> pipe[1], &byte, sizeof(int)); /* for header, and passing on chunk */
}

msg_buff_t
channel_local_pop_chunk(channel_t chan){
  int byte;
  const void *head;
  msg_buff_t chunk;

  /* ack for reading header */
  std_read(chan -> pipe[0], &byte, sizeof(int));
  
  /* read from blocking finite queue */
  pthread_mutex_lock(&chan -> lock);
  chunk = msg_buff_list_popleft(chan -> buff_queue);
    
  /* if queue is not full anymore, signal */
  if(msg_buff_list_size(chan -> buff_queue) == CHANNEL_MSG_QUEUE_LIMIT - 1)
    pthread_cond_broadcast(&chan -> cond);

  pthread_mutex_unlock(&chan -> lock);
  
  assert(chunk);
    
  /* pack header  */
  head = *msg_buff_head(chunk);
  msg_info_unpack(chan -> msg_info, &head);

  /* we pop entire chunk, so there is no remainder */
  chan -> msg_info -> remain = 0;
    
  return chunk;
}
