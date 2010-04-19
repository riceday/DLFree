#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <assert.h>

#include <std/std.h>
#include <struct/rtable.h>
#include "impl/ioman.h"
#include "impl/msg.h"
#include "impl/comm.h"

static int
Max(int x, int y){
  return x > y ? x : y;
}

static int
Min(int x, int y){
  return x < y ? x : y;
}

static double
get_curr_time(){
  struct timeval tv;
  std_gettimeofday(&tv, NULL);
  return tv.tv_sec + tv.tv_usec *1e-6;
}

#if COMM_MONITOR_RECV_BAND

static void*
recved_bytes_monitor(void* _node){
  comm_node_t node = (comm_node_t)_node;
  long prev_rx = 0, cur_rx;
  long prev_tx = 0, cur_tx;
  double ts, t0, t1, band_rx, band_tx;
  
  ts = get_curr_time();
  t0 = get_curr_time();
  while(node -> monitor_run){
    t1 = get_curr_time();

    ioman_get_traffic_info(node -> man, &cur_rx, &cur_tx);
    band_tx = (cur_tx - prev_tx) / (t1 - t0);
    band_rx = (cur_rx - prev_rx) / (t1 - t0);
    fprintf(stderr, "%d:, %.3f, RX band, %.3f, [KB/s]\n", node -> node_id, t1 - ts, band_rx / 1000);
    fprintf(stderr, "%d:, %.3f, TX band, %.3f, [KB/s]\n", node -> node_id, t1 - ts, band_tx / 1000);
    prev_tx = cur_tx;
    prev_rx = cur_rx;
    t0 = t1;
    std_usleep(500000);
  }
  return NULL;
}

static void*
send_buffer_monitor(void* _node){
  comm_node_t node = (comm_node_t)_node;
  long chunks;
  
  while(node -> monitor_run){

    ioman_get_send_buffer_info(node -> man, &chunks);
    fprintf(stderr, "%d: send buff, %ld, [KB]\n", node -> node_id, chunks * (COMM_DATA_CHUNK_SIZE / 1000) );
    std_sleep(1);
  }
  return NULL;
}

#endif // COMM_MONITOR_RECV_BAND

comm_node_t
comm_node_create(int node_id){
  int idx;
  comm_node_t node = (comm_node_t)std_malloc(sizeof(comm_node));
  node -> node_id = node_id;
  node -> man = ioman_create(node_id, node, COMM_MAX_PEER);

  node -> pending_conns = channel_hash_map_create(COMM_HASH_SIZE);

  std_pthread_mutex_init(&node -> lock, NULL);
  std_pthread_cond_init(&node -> cond, NULL);

  for(idx = 0; idx < COMM_MAX_PEER; idx++){
    node -> rtts[idx] = 0.0;
    node -> rts[idx] = NULL;
  }
  node -> num_rts = 0;

  node -> data_msg_chunk_size = COMM_DATA_CHUNK_SIZE;
  node -> data_msg_sid = 0;

  node -> data_msg_recvd = 0;

  node -> max_width = 0;
  node -> min_width = INT_MAX;
  
  node -> data_msg_map = data_msg_hash_map_create(COMM_HASH_SIZE);
  node -> recvd_data_msg_queues = (data_msg_list_t*) std_calloc(COMM_MAX_PEER, sizeof(data_msg_list_t));

  node -> recvd_bytes = 0;
  /* start bandwidth monitor */
#if COMM_MONITOR_RECV_BAND
  node -> monitor_run = 1;
  std_pthread_create(&node -> monitor, NULL, recved_bytes_monitor, (void*)node);
#endif

  ioman_start(node -> man);
  
  return node;
}

void
comm_node_destroy(comm_node_t node){
  int idx;
  data_msg_t unread_msg;
  data_msg_list_t msg_queue;
  
  ioman_stop(node -> man);
  ioman_destroy(node -> man);
  channel_hash_map_destroy(node -> pending_conns);
  std_pthread_mutex_destroy(&node -> lock);
  std_pthread_cond_destroy(&node -> cond);

  for(idx = 0; idx < COMM_MAX_PEER; ++ idx){
    if(node -> rts[idx] != NULL){
      overlay_rtable_destroy(node -> rts[idx]);
      node -> rts[idx] = NULL;
    }
  }
  
  data_msg_hash_map_destroy(node -> data_msg_map);

  /* clean up all unread received messages */
  for(idx = 0; idx < COMM_MAX_PEER; ++idx){
    msg_queue = node -> recvd_data_msg_queues[idx];
    if(msg_queue){
      while(data_msg_list_size(msg_queue)){
	unread_msg = data_msg_list_pop(msg_queue);
	data_msg_destroy(unread_msg);
      }
      data_msg_list_destroy(msg_queue);
    }
  }
  std_free(node -> recvd_data_msg_queues);

  /* stop bandwidth monitor */
#if COMM_MONITOR_RECV_BAND
  node -> monitor_run = 0;
  std_pthread_join(node -> monitor, NULL);
#endif
  
  std_free(node);
}

int
comm_node_listen_port(comm_node_t node){
  return ioman_listen_port(node -> man);
}

sid_t
comm_node_get_new_sid(comm_node_t node){
  sid_t sid;
  std_pthread_mutex_lock(&node -> lock);
  sid = (node -> data_msg_sid ++);
  std_pthread_mutex_unlock(&node -> lock);
  sid <<= 20;

  assert(node -> node_id < (1 << 20));
  sid += node -> node_id;
  
  return sid;
}

unsigned long
comm_node_async_connect(comm_node_t node, const char* addr, int port){
  unsigned long handle;
  channel_t chan;
  std_pthread_mutex_lock(&node -> lock);

  handle = channel_hash_map_new_key(node -> pending_conns);
  if(ioman_new_connection(node -> man, addr, port, &chan) == IOMAN_CONNECT_ERR){
    chan = NULL;
    fprintf(stderr, "%d: connect failed to (%s, %d)\n", node -> node_id, addr, port);
  }

  /* register channel in pending table */
  channel_hash_map_add(node -> pending_conns, handle, chan);

  std_pthread_mutex_unlock(&node -> lock);

  return handle;
}

int
comm_node_connect_wait(comm_node_t node, unsigned long handle, int* dst_id){
  channel_t chan;
  std_pthread_mutex_lock(&node -> lock);

  /* wait until error or peer is known */
  /*  TODO: a bit ad-hoc... */
  while((chan = channel_hash_map_find(node -> pending_conns, handle)) != NULL &&
	!channel_is_established(chan)){
        /* (chan -> peer_id == CHANNEL_PEER_UNKNOWN)){ */
    std_pthread_cond_wait(&node -> cond, &node -> lock);
  }

  channel_hash_map_pop(node -> pending_conns, handle);
  
  std_pthread_mutex_unlock(&node -> lock);

  if(chan != NULL){ /* success */
    *dst_id = chan -> peer_id;
    return 0;
  }
  else{ /* failure */
    fprintf(stderr, "%d: connect failed handle: %ld\n", node -> node_id, handle);
    return -1;
  }
}

void
comm_node_notify_connect(comm_node_t node, channel_t chan){
  msg_info_t minfo = msg_info_create();
  
  /* send first ping message */
  minfo -> kind   = MSG_TYPE_PING0;
  minfo -> dst_id = -1; /* unknown at this time */
  minfo -> src_id = node -> node_id;
  minfo -> len    = 0;

  channel_send_msg(chan, minfo, NULL, 0);
  
  msg_info_destroy(minfo);
}

void
comm_node_notify_success(comm_node_t node, channel_t chan){
  /* just notify */
  std_pthread_mutex_lock(&node -> lock);
  channel_make_established(chan);
  std_pthread_cond_broadcast(&node -> cond);
  std_pthread_mutex_unlock(&node -> lock);
}

void
comm_node_notify_failure(comm_node_t node, channel_t chan){
  std_pthread_mutex_lock(&node -> lock);

  /* remove channel from pending channel and notify */
  channel_hash_map_remove(node -> pending_conns, chan);
  
  std_pthread_cond_broadcast(&node -> cond);
  std_pthread_mutex_unlock(&node -> lock);
}

void
comm_node_rtt_measure_start(comm_node_t node, int dst_id){
  assert(dst_id < COMM_MAX_PEER);
  node -> rtts[dst_id] = get_curr_time();
}

void
comm_node_rtt_measure_end(comm_node_t node, int dst_id){
  assert(dst_id < COMM_MAX_PEER);
  double t = node -> rtts[dst_id];
  assert(t != 0); // should have measured before
  node -> rtts[dst_id] = get_curr_time() - t;
  /* printf("%d: rtt -> %d :%.3f[ms]\n", node -> node_id, dst_id, node -> rtts[dst_id] * 1e3); */
}

void
comm_node_bcast_msg(comm_node_t node, int msg_kind, const void *buff, int len){
  ioman_bcast_msg(node -> man, msg_kind, buff, len);
}

int
comm_node_register_rt(comm_node_t node, const void** buff){
  overlay_rtable_t rt = overlay_rtable_create_by_unpack(buff);
  overlay_rtable_entry_t entry;
  int pid, updated = 0;
  std_pthread_mutex_lock(&node -> lock);
  if(node -> rts[rt -> srcpid] == NULL){ /* if not received yet */
    node -> rts[rt -> srcpid] = rt;
    updated = 1;
    node -> num_rts ++;

    /* from received rt: calculate max/min bandwidth path in network */
    for(pid = 0; pid < overlay_rtable_size(rt); pid++){
      if(pid == rt -> srcpid) continue;
      entry = overlay_rtable_get_entry(rt, pid);
      node -> max_width = Max(node -> max_width, entry -> width);
      node -> min_width = Min(node -> min_width, entry -> width);
    }
    
    std_pthread_cond_broadcast(&node -> cond); /* notify new information */
  }else{
    overlay_rtable_destroy(rt); /* if already here, unnecessary */
  }
  std_pthread_mutex_unlock(&node -> lock);

  return updated;
}

void
comm_node_exchange_rt(comm_node_t node, overlay_rtable_t rt, int num_peers){
  int bufflen = overlay_rtable_pack_len(rt);
  void *buff = (void*) std_calloc(bufflen, 1);
  void *p;
  
  /* printf("%d has %d conns\n", node -> node_id, ioman_get_nsocks(node -> man));fflush(stdout); */
  /* broadcast rt */
  p = buff;
  overlay_rtable_pack(rt, &p);
  assert(node -> node_id == rt -> srcpid && node -> rts[rt -> srcpid] == NULL);
  node -> rts[rt -> srcpid] = rt;
  comm_node_bcast_msg(node, MSG_TYPE_RT, buff, bufflen);
  std_free(buff);

  /* wait for others */
  std_pthread_mutex_lock(&node -> lock);
  node -> num_rts ++; /* for my own rt */
  while(num_peers > node -> num_rts)
    std_pthread_cond_wait(&node -> cond, &node -> lock); /* notify new information */
  
  std_pthread_mutex_unlock(&node -> lock);
}

int
comm_node_lookup_rt(comm_node_t node, int src_pid, int dst_pid){
  overlay_rtable_entry_t entry;
  overlay_rtable_t rt = node -> rts[src_pid];
  int i;
  assert(rt != NULL);
  entry = overlay_rtable_get_entry(rt, dst_pid);
  assert(entry != NULL);
  /* go through path and find this node */
  /* next hop comes after that */
  for(i = 0; i < entry -> hops; i++){
    if(entry -> path[i] == node -> node_id){
      return entry -> path[i + 1]; /* return next hop */
    }
  }
  return -1;
}

void
comm_node_handle_msg(comm_node_t node, channel_t chan, const msg_info_t msg_info, const msg_buff_t buff){
  int src_id = msg_info -> src_id;
  const void* rawbuff = *msg_buff_head(buff);
  switch(msg_info -> kind){
  case MSG_TYPE_PING0: /* acceptor */
    //printf("%d: got PING0\n", node -> node_id);fflush(stdout);
    ioman_register_channel(node -> man, src_id, chan); /* register peer id */
    
    comm_node_rtt_measure_start(node, src_id);
    /* send ping */
    ioman_send_msg(node -> man, MSG_TYPE_PING1, src_id, NULL, 0);
    break;
  case MSG_TYPE_PING1: /* connector */
    //printf("%d: got PING1\n", node -> node_id);fflush(stdout);
    ioman_register_channel(node -> man, src_id, chan); /* register peer id */
    comm_node_rtt_measure_start(node, src_id);
    /* send pong */
    ioman_send_msg(node -> man, MSG_TYPE_PONG0, src_id, NULL, 0);
    break;
  case MSG_TYPE_PONG0: /* acceptor */
    //printf("%d: got P0NG0\n", node -> node_id);fflush(stdout);
    comm_node_rtt_measure_end(node, src_id);
    /* send pong */
    ioman_send_msg(node -> man, MSG_TYPE_PONG1, src_id, NULL, 0);
    break;
  case MSG_TYPE_PONG1: /* connector */
    //printf("%d: got P0NG1\n", node -> node_id);fflush(stdout);
    comm_node_rtt_measure_end(node, src_id);

    /* connection process completes here */
    comm_node_notify_success(node, chan);
    break;
  case MSG_TYPE_RT:
    if(comm_node_register_rt(node, &rawbuff)){
      comm_node_bcast_msg(node, MSG_TYPE_RT, *msg_buff_head(buff), msg_buff_len(buff));
    }
    break;
  default:
    fprintf(stderr, "comm_node_handle_msg: unknown msg type: ");
    msg_info_print(msg_info);
    exit(1);
  }
}

/* void */
/* comm_node_handle_chunk_header(comm_node_t node, channel_t chan, const msg_info_t header){ */
/*   double t; */
/*   int sid = header -> sid; */
/*   data_msg_t data = data_msg_hash_map_find(node -> data_msg_map, sid); */

/*   assert(sid != -1); /\* valid sid *\/ */
  
/*   /\* create new data msg if new session *\/ */
/*   if(data == NULL){ */
/*     t = get_curr_time(); */
/*     data = data_msg_create(header -> tot_len, header -> src_id, t); */
/*     data_msg_hash_map_add(node -> data_msg_map, sid, data); */
/*     /\* printf("%d: got header src: %d\n", node -> node_id, header -> src_id);fflush(stdout); *\/ */
/*   } */

/*   /\* give extra room for header *\/ */
/*   data -> len += CHANNEL_MSG_HEADERLEN; */
/* } */

void
comm_node_setup_chunk(comm_node_t node, channel_t chan, const msg_info_t header){
  double t;
  sid_t sid = header -> sid;
  data_msg_t data = data_msg_hash_map_find(node -> data_msg_map, sid);

  assert(sid != -1); /* valid sid */
  
  /* create new data msg if new session */
  if(data == NULL){
    t = get_curr_time();
    data = data_msg_create(header -> tot_len, header -> src_id, t);
    data_msg_hash_map_add(node -> data_msg_map, sid, data);
    /* printf("%d: got header src: %d\n", node -> node_id, header -> src_id);fflush(stdout); */
  }

/*   setup channel chunk using data message buffer */
/*   writing directly to buffer will reduce copying */
  channel_setup_chunk_with_buff(chan, data_msg_buff_tail(data));
}

void
comm_node_deliver_chunk(comm_node_t node, data_msg_t msg){
  data_msg_list_t msg_queue;
/*     fprintf(stdout, "%d: %s\n", node -> node_id, (char*)usr_buff);fflush(stdout); */
/*     usr_buff = data_msg_destroy(data); */
/*     free(usr_buff); */

    std_pthread_mutex_lock(&node -> lock);

    /* access message queue per source id */
    msg_queue = node -> recvd_data_msg_queues[msg -> src_id];
    if(msg_queue == NULL){
      msg_queue = data_msg_list_create();
      node -> recvd_data_msg_queues[msg -> src_id] = msg_queue;
    }

    data_msg_list_append(msg_queue, msg);
    ++ node -> data_msg_unacked_recvd;

    /* stats */
    ++ node -> data_msg_recvd;
    
    std_pthread_cond_broadcast(&node -> cond);
    std_pthread_mutex_unlock(&node -> lock);
}

void
comm_node_recv_data(comm_node_t node, int src_id, void** buff, int* buffsize){
  data_msg_t msg = 0;
  data_msg_list_t msg_queue = 0;
  buff = 0;

  std_pthread_mutex_lock(&node -> lock);

  while(1){
    msg_queue = node -> recvd_data_msg_queues[src_id];
    
    if(msg_queue != NULL && data_msg_list_head(msg_queue) != data_msg_list_end(msg_queue)){
      msg = data_msg_list_popleft(msg_queue);
      assert(msg -> src_id == src_id); /* srcid of msg queue and of message should match */
      *buffsize = msg -> len;
      *buff     = data_msg_destroy(msg);
      -- node -> data_msg_unacked_recvd;
      assert(node -> data_msg_unacked_recvd >= 0);
      break;
    }
    else{
      std_pthread_cond_wait(&node -> cond, &node -> lock);
    }
  }
  
  std_pthread_mutex_unlock(&node -> lock);

  return;
}

void
comm_node_recv_any_data(comm_node_t node, int* src_id, void** buff, int* buffsize){
  int tmp_nodeid;
  data_msg_t msg = 0;
  data_msg_list_t msg_queue = 0;

  std_pthread_mutex_lock(&node -> lock);

  while(1){

    if(node -> data_msg_unacked_recvd > 0){
      for(tmp_nodeid = 0; tmp_nodeid < COMM_MAX_PEER; ++tmp_nodeid){
	msg_queue = node -> recvd_data_msg_queues[tmp_nodeid];
	if(msg_queue != NULL && data_msg_list_size(msg_queue) > 0){
	  msg = data_msg_list_popleft(msg_queue);
	  *src_id = msg -> src_id;
	  assert(*src_id == tmp_nodeid); /* srcid of msg queue and of message should match */
	  *buffsize = msg -> len;
	  *buff     = data_msg_destroy(msg);
	  break;
	}
      }
      -- node -> data_msg_unacked_recvd;
      assert(node -> data_msg_unacked_recvd >= 0);
      assert(buff != 0); /* message should have been retrieved by now */
      break;
    }
    else{
      std_pthread_cond_wait(&node -> cond, &node -> lock);
    }
  }
  
  std_pthread_mutex_unlock(&node -> lock);

  return;
}

void
comm_node_handle_chunk(comm_node_t node, channel_t chan, const msg_info_t header, const msg_buff_t chunk){
  sid_t sid = header -> sid;
  data_msg_t data;
  double dt;

  assert(sid != -1); /* valid sid */
  data = data_msg_hash_map_find(node -> data_msg_map, sid);
  assert(data != NULL); /* should have been created by handle_chunk_header */

  node -> recvd_bytes += msg_buff_len(chunk); /* for stats */

  if(data_msg_push_chunk(data, header, chunk) == DATA_MSG_FULL){
    dt = get_curr_time() - (data -> start_time);
/*     printf("%d: got data msg src: %d len: %d band: %.3f[MB/s]\n", node -> node_id, header -> src_id, data -> len, (data -> len) * 1e-6 / dt);fflush(stdout); */

    data_msg_hash_map_pop(node -> data_msg_map, sid);

    comm_node_deliver_chunk(node, data);
  }
/*   else{ */
/*     printf("%d: got chunk src: %d len: %d/%d\n", node -> node_id, header -> src_id, data -> recvd, data -> len);fflush(stdout); */
/*   } */
}

#if COMM_ADJUST_CHUNK_SIZE
static int
comm_node_calc_chunk_size(comm_node_t node, int dst_id){
  overlay_rtable_t rt = node -> rts[node -> node_id];
  overlay_rtable_entry_t entry = overlay_rtable_get_entry(rt, dst_id);
  return (int)((float)entry -> width / node -> max_width * node -> data_msg_chunk_size);
}
#endif

double
comm_node_peer_rtt(comm_node_t node, int dst_id){
  return node -> rtts[dst_id];
}

void
comm_node_send_data(comm_node_t node, int dst_id, const void *buff, int len){
  sid_t sid = comm_node_get_new_sid(node);
  int seq;
  int remain, off;
  int size;

#if COMM_ADJUST_CHUNK_SIZE
  int CHUNK_SZ = comm_node_calc_chunk_size(node, dst_id);
#else
  int CHUNK_SZ = node -> data_msg_chunk_size;
#endif
  

  for(seq = 0, off = 0, remain = len; remain > 0; seq ++){
    size = CHUNK_SZ > remain ? remain : CHUNK_SZ;
    ioman_send_chunk(node -> man, dst_id, sid, len, seq, buff + off, size);
    remain -= size;
    off += size;
    /* printf("%d: comm_node_send_data %d/%d -> %d\n", node -> node_id, off, len, dst_id);fflush(stdout); */
  }
}

/* void */
/* comm_node_wait_data(comm_node_t node, int count){ */
/*   std_pthread_mutex_lock(&node -> lock); */
/*   while(node -> data_msg_recvd < count) */
/*     std_pthread_cond_wait(&node -> cond, &node -> lock); */
/*   node -> data_msg_recvd -= count; */
/*   std_pthread_mutex_unlock(&node -> lock); */
/* } */
