#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <linux/tcp.h>

#include <std/std.h>
#include "impl/sock.h"
#include "impl/comm.h"
#include "impl/ioman.h"

#define SetMax(x, a, b) ( x = (a) > (b) ? (a) : (b) )

ioman_t
ioman_create(int node_id, comm_node_t comm, int maxpeers){
  const int buff_size = IOMAN_NOTIFYPIPE_BUFF_SIZE;
  ioman_t man = (ioman_t)std_malloc(sizeof(ioman));

  man -> node_id = node_id;
  man -> comm = comm;
  
  std_gethostname(man -> node_hostname, sizeof(man -> node_hostname));

  //printf("%d: %s\n", man -> node_id, man -> node_hostname);fflush(stdout);

  /* std_pipe(man -> pipe_R); */
  /* std_socketpair(AF_UNIX, SOCK_STREAM, 0, man -> pipe_R); */
  std_tcp_socketpair(man -> pipe_R);
  std_setsockopt(man -> pipe_R[0], SOL_SOCKET, SO_RCVBUF, &buff_size, sizeof(buff_size));
  std_setsockopt(man -> pipe_R[1], SOL_SOCKET, SO_SNDBUF, &buff_size, sizeof(buff_size));

  man -> channels = channel_list_create();

  man -> channel_map = (channel_t*) std_calloc(maxpeers, sizeof(channel_t));
  man -> maxpeers = maxpeers;

  man -> local_chan = channel_local_create(node_id);
  man -> lsock = NULL;

  std_pthread_mutex_init(&man -> lock, NULL);

  man -> maxfd = -1;
  FD_ZERO(&man -> R_fds);
  FD_ZERO(&man -> W_fds);
  man -> fdset_valid = IOMAN_FDSET_CACHE_INVALID;

/*   man -> use_cache = 0; */
/*   man -> use_total = 0; */
  
  return man;
}

void
ioman_destroy(ioman_t man){
  std_close(man -> pipe_R[0]);
  std_close(man -> pipe_R[1]);
  
  channel_list_destroy(man -> channels);
  std_free(man -> channel_map);
    
  std_pthread_mutex_destroy(&man -> lock);

  channel_local_destroy(man -> local_chan);
  
  std_free(man);
}

void
ioman_register_channel(ioman_t man, int dst_id, channel_t chan){
  assert(man -> channel_map[dst_id] == NULL);
  assert(chan -> peer_id == CHANNEL_PEER_UNKNOWN);
  man -> channel_map[dst_id] = chan;
/*   printf("%d: registered chan: %d\n", man -> node_id, dst_id);fflush(stdout); */
  chan -> peer_id = dst_id; /* register peer id */
}

void
ioman_delete_channel(ioman_t man, channel_t chan){
  int dstid;

  dstid = chan -> peer_id;
  /* if channel has been registered with dstid, remove that */
  if(dstid != CHANNEL_PEER_UNKNOWN){
    assert(man -> channel_map[dstid] == chan);
    man -> channel_map[dstid] = NULL;
  }
  channel_destroy(chan);

  man -> fdset_valid = IOMAN_FDSET_CACHE_INVALID;
}

int
ioman_get_nsocks(ioman_t man){
  return channel_list_size(man -> channels);
}

void
ioman_get_traffic_info(ioman_t man, long *rx_count, long *tx_count){
  channel_t chan;
  int dstid;

  *rx_count = 0; *tx_count = 0;
  for(dstid = 0; dstid < man -> maxpeers; dstid++){
    if((chan = man -> channel_map[dstid]) != NULL){
      *rx_count += chan -> rx_count;
      *tx_count += chan -> tx_count;
    }
  }
}

void
ioman_get_send_buffer_info(ioman_t man, long *count){
  channel_t chan;
  int dstid;

  *count = 0;
  for(dstid = 0; dstid < man -> maxpeers; dstid++){
    if((chan = man -> channel_map[dstid]) != NULL){
      *count += msg_buff_list_size(chan -> buff_queue);
    }
  }
}

void
ioman_channel_tcp_info_print(ioman_t man, int dst_id){
  struct tcp_info info;
  channel_t chan = man -> channel_map[dst_id];
  assert(chan != NULL);

  info = sock_tcp_info(channel_get_sock(chan));

  printf("%d: channel tcp info dstid: %d: RTOs = %u, Probes = %u, Backoffs = %u\
 RTO = %u msec, RTT = %u msec, RTT_var = %u msec\
 lost packets = %u, sacked packets = %u, retransmitted packets = %u\
 SND_MSS = %u, RCV_MSS = %u, ssthresh = %u, cwnd = %u\n",
	 man -> node_id,
	 dst_id,
	 info.tcpi_retransmits, info.tcpi_probes, info.tcpi_backoff,
	 (info.tcpi_rto/1000), (info.tcpi_rtt/1000),
	 (info.tcpi_rttvar/1000),
	 info.tcpi_lost, info.tcpi_sacked, info.tcpi_total_retrans,
	 info.tcpi_snd_mss, info.tcpi_rcv_mss,
	 info.tcpi_snd_ssthresh, info.tcpi_snd_cwnd);
}

void
ioman_finalize(ioman_t man){
  channel_t chan;
/*   printf("%d: ioman_finalize\n", man -> node_id);fflush(stdout); */
  sock_destroy(man -> lsock);
  while(channel_list_size(man -> channels)){
    chan = channel_list_pop(man -> channels);
    ioman_delete_channel(man, chan);
  }

/*   printf("%d: ioman_finalize: cache usage: %d/%d %.3f\n", man ->node_id, */
/* 	 man -> use_cache, man -> use_total, ((float)man -> use_cache) / man -> use_total); */
}

void
ioman_notify_finalize(ioman_t man){
  int e = IOMAN_EVENT_FINALIZE;

  std_pthread_mutex_lock(&man -> lock);
  std_write(man -> pipe_R[1], &e, sizeof(int));
  std_pthread_mutex_unlock(&man -> lock);
}

void
ioman_stop(ioman_t man){
  ioman_notify_finalize(man);
  std_pthread_join(man -> handler, NULL);
}

int
ioman_listen_port(ioman_t man){
  assert(man -> lsock != NULL);
  return sock_port(man -> lsock);
}

void
ioman_notify_newchan(ioman_t man, channel_t chan){
  int e = IOMAN_EVENT_NEWCHAN;

  std_pthread_mutex_lock(&man -> lock);
  std_write(man -> pipe_R[1], &e, sizeof(int));
  std_write(man -> pipe_R[1], &chan, sizeof(channel_t));
  std_pthread_mutex_unlock(&man -> lock);
  
}

int
ioman_new_connection(ioman_t man, const char* addr, int port, channel_t *chan){
  sock_t sk = connect_sock_create(addr, port);

  if(sock_connect(sk) == SOCK_CONNECT_ERR){
    sock_destroy(sk);
    return IOMAN_CONNECT_ERR;
  }

  *chan = channel_setup_create(sk);
  ioman_notify_newchan(man, *chan);
  
  return IOMAN_CONNECT_OK;
}

void
ioman_handle_event_newchan(ioman_t man){
  channel_t chan;
  int n = std_read(man -> pipe_R[0], &chan, sizeof(channel_t));
  assert(n == sizeof(channel_t));

  /* add channel to list */
  channel_list_append(man -> channels, chan);

  man -> fdset_valid = IOMAN_FDSET_CACHE_INVALID;
}

void
ioman_notify_newmsg(ioman_t man){
  int e = IOMAN_EVENT_NEWMSG;

  std_pthread_mutex_lock(&man -> lock);
  std_write(man -> pipe_R[1], &e, sizeof(int));
  std_pthread_mutex_unlock(&man -> lock);
}

void /* to be used ONLY by ioman thread, this does not do notify event */
ioman_sys_send_msg(ioman_t man, channel_t chan, int kind, int dst_id, const void* buff, int len){
  msg_info_t minfo = msg_info_create();
  
  /* pack message info */
  minfo -> kind   = kind;
  minfo -> dst_id = dst_id;
  minfo -> src_id = man -> node_id;
  minfo -> len    = len;

  channel_send_msg(chan, minfo, buff, len);

  msg_info_destroy(minfo);
}

void
ioman_handle_event_newmsg(ioman_t man){
  man -> fdset_valid = IOMAN_FDSET_CACHE_INVALID;
}

void
ioman_handle_event_bcastmsg(ioman_t man){
  int pid;
  void *buff;
  int kind, len;
  channel_t chan;
  std_read(man -> pipe_R[0], &kind, sizeof(int));
  std_read(man -> pipe_R[0], &buff, sizeof(void*));
  std_read(man -> pipe_R[0], &len, sizeof(int));

  for(pid = 0; pid < man -> maxpeers; pid++){
    if((chan = man -> channel_map[pid]) != NULL){
      ioman_sys_send_msg(man, chan, kind, pid, buff, len);
    }
  }
  std_free(buff); /* free what was alloc-ed in bcast_msg() */

  man -> fdset_valid = IOMAN_FDSET_CACHE_INVALID;
}

int
ioman_handle_event(ioman_t man){
  int e;
  int n = std_read(man -> pipe_R[0], &e, sizeof(int));
  assert(n == sizeof(int));

  switch(e){
  case IOMAN_EVENT_FINALIZE:
    break;
  case IOMAN_EVENT_NEWMSG:
    ioman_handle_event_newmsg(man);
    break;
  case IOMAN_EVENT_BCASTMSG:
    ioman_handle_event_bcastmsg(man);
    break;
  case IOMAN_EVENT_NEWCHAN:
    ioman_handle_event_newchan(man);
    break;
  default:
    fprintf(stderr, "ioman_handle_event: invalid event type: %d\n", e);
    exit(1);
  }
  return e;
}

void
ioman_set_fds(ioman_t man, fd_set *R_fds, fd_set *W_fds, int *maxfd){
  channel_t chan;
  channel_list_cell_t chan_cell;
  int fd;

  //printf("%d: ioman_set_fds\n", man -> node_id);fflush(stdout);

/*   man -> use_total ++; */
  if(man -> fdset_valid == IOMAN_FDSET_CACHE_VALID){
    *maxfd = man -> maxfd;
    *R_fds = man -> R_fds;
    *W_fds = man -> W_fds;

/*     man -> use_cache ++; */
  }else{
    FD_ZERO(R_fds);
    FD_ZERO(W_fds);
    
    /* notify pipe */
    FD_SET(man -> pipe_R[0], R_fds);
    *maxfd = man -> pipe_R[0];
    
    /* listen sock */
    fd = sock_fileno(man -> lsock);
    FD_SET(fd, R_fds);
    SetMax(*maxfd, *maxfd, fd);
    
    /* local channel */
    if(channel_is_readable(man -> local_chan)){
      fd = sock_fileno(channel_get_sock(man -> local_chan));
      FD_SET(fd, R_fds);
      SetMax(*maxfd, *maxfd, fd);
    }
    
    /* channels */
    for(chan_cell = channel_list_head(man -> channels);
	chan_cell != channel_list_end(man -> channels);
	chan_cell = channel_list_cell_next(chan_cell)){
      
      chan = channel_list_cell_data(chan_cell);
      
      if(channel_is_readable(chan)){
	fd = sock_fileno(channel_get_sock(chan));
	FD_SET(fd, R_fds);
	SetMax(*maxfd, *maxfd, fd);
      }
      
      if(channel_is_writable(chan)){
	fd = sock_fileno(channel_get_sock(chan));
	FD_SET(fd, W_fds);
	SetMax(*maxfd, *maxfd, fd);
      }
    }

    /* create cache */
    man -> maxfd = *maxfd;
    man -> R_fds = *R_fds;
    man -> W_fds = *W_fds;
    man -> fdset_valid = IOMAN_FDSET_CACHE_VALID;
  }

}

void
ioman_check_set_fds(ioman_t man, fd_set *fds){
  channel_t chan;
  channel_list_cell_t chan_cell;
  int fd;

  /* notify pipe */
  printf("set fds: [");
  if(FD_ISSET(man -> pipe_R[0], fds))
    printf("notify, ");

  /* listen sock */
  if(FD_ISSET(sock_fileno(man -> lsock), fds))
    printf("lsock, ");

  /* local channel */
  if(FD_ISSET(sock_fileno(channel_get_sock(man -> local_chan)), fds))
    printf("local, ");

  /* channels */
  for(chan_cell = channel_list_head(man -> channels);
      chan_cell != channel_list_end(man -> channels);
      chan_cell = channel_list_cell_next(chan_cell)){

    chan = channel_list_cell_data(chan_cell);
    
    if(channel_is_readable(chan)){
      fd = sock_fileno(channel_get_sock(chan));
      if(FD_ISSET(fd, fds))
	printf("%d, ", fd);
    }
  }
  printf("]\n");
  
}

channel_t
ioman_get_nexthop_channel(ioman_t man, int src_id, int dst_id){
  channel_t nexthop;
  int nextpid = comm_node_lookup_rt(man -> comm, src_id, dst_id);
  assert(nextpid != -1);
  nexthop = man -> channel_map[nextpid];
  if(nexthop == NULL){
    fprintf(stdout, "%d: NEXTHOP: %d -> %d: next: %d\n", man -> node_id, src_id, dst_id, nextpid);
    fflush(stdout);
  }
  assert(nexthop != NULL);
  return nexthop;
}

int
ioman_process_local_channel_read(ioman_t man, channel_t chan){
  channel_t next_chan;

  /* if need to pop chunk and acquire next */

  /* pop off chunk from local message queue */
  /* equivlent to reading header and chunk body */
  chan -> curr_buff = channel_local_pop_chunk(chan);
  
  //printf("%d: local_channel_read: ", man -> node_id);msg_info_print(chan -> msg_info); fflush(stdout);
  assert(chan -> msg_info -> src_id != chan -> msg_info -> dst_id);
  
  /* acquire connect to next hop */
  next_chan = ioman_get_nexthop_channel(man,
					chan -> msg_info -> src_id,
					chan -> msg_info -> dst_id);
  
  /* pass on to responsible next hop */
  /* pipeline that can fail to push, if so, need to block */
  channel_pipeline_chunk(chan, next_chan);
  
  man -> fdset_valid = IOMAN_FDSET_CACHE_INVALID;
  
  return 0;
}

int
ioman_process_channel_read(ioman_t man, channel_t chan){
  int msg_kind;
  int stat;
  channel_t next_chan;
  msg_buff_t msg;

  /* read header to allocate buffer */
/*   if(chan -> msg_info -> remain == 0){ */
  if(chan -> curr_buff == NULL){
    //printf("%d: reading header\n", man -> node_id); fflush(stdout);
    switch(channel_read_header(chan, &msg_kind)){
    case CHANNEL_READ_ERR:
      return -1;
    case CHANNEL_READ_EAGAIN:
      return 0;
    }

/*     printf("%d: read header: ", man -> node_id); msg_info_print(chan -> msg_info); fflush(stdout); */
    
    if(msg_kind == MSG_TYPE_DATA){
      

      if(chan -> msg_info -> dst_id == man -> node_id){
	/* upcall and handle allocation for full message and channel chunk */
	comm_node_setup_chunk(man -> comm, chan, chan -> msg_info);
      }else{
	/* alloc chunk-size buff */
	channel_setup_chunk(chan); 
      }
    }
    else
      channel_setup_msg(chan); /* alloc FULL msg size buff */
  }

  /* printf("%d: reading body: ", man -> node_id); msg_info_print(chan -> msg_info); fflush(stdout); */
  /* read body */
  if(chan -> msg_info -> kind == MSG_TYPE_DATA){
    /* pass on to next node */
    if((stat = channel_read_chunk(chan, &msg)) == CHANNEL_READ_DONE){
      if(chan -> msg_info -> dst_id == man -> node_id){
	/* upcall to pass chunk*/
	comm_node_handle_chunk(man -> comm, chan, chan -> msg_info, msg);
	chan -> curr_buff = NULL;
      }else{
	next_chan = ioman_get_nexthop_channel(man,
					      chan -> msg_info -> src_id,
					      chan -> msg_info -> dst_id);

	channel_pipeline_chunk(chan, next_chan);

	man -> fdset_valid = IOMAN_FDSET_CACHE_INVALID;
      }

      assert(chan -> msg_info -> remain == 0);
    }
  }
  else{
    /* receive message */
    if((stat = channel_read_msg(chan, &msg)) == CHANNEL_READ_DONE){
      /* only time dst id is ambiguous is when first ping is sent */
      assert(chan -> msg_info -> dst_id == man -> node_id || chan -> msg_info -> kind == MSG_TYPE_PING0);

      
      comm_node_handle_msg(man -> comm, chan, chan -> msg_info, msg);

      /* clean-up buff */
      msg_buff_destroy(msg);
    }
  }

  /* printf("%d: read body end: remaining %d/%d\n", man -> node_id, chan -> msg_info -> remain, chan -> msg_info -> len); fflush(stdout); */

  if(stat == CHANNEL_READ_ERR)
    return -1;
  else
    return 0;
}

int
ioman_process_channel_write(ioman_t man, channel_t chan){
  int stat, unblock;
  /* simply write as much you can */
  switch(chan -> state){
  case CHANNEL_INIT:
    fprintf(stderr, "ioman_process_channel_write: channel in INIT state\n");
    exit(1);
  case CHANNEL_SETUP:
    if(channel_make_active(chan) == CHANNEL_ACTIVATE_ERR){
      comm_node_notify_failure(man -> comm, chan);
      return -1;
    }

    comm_node_notify_connect(man -> comm, chan);

    man -> fdset_valid = IOMAN_FDSET_CACHE_INVALID;
    
    break;
  default: /* ACTIVE and BLOCKING */
    if((stat = channel_write(chan, &unblock)) == CHANNEL_WRITE_ERR)
      return -1;

    /* need to re-set fdset if send-queue becomes empty or some producer is unblocked */
    if(stat == CHANNEL_WRITE_OK || unblock)
      man -> fdset_valid = IOMAN_FDSET_CACHE_INVALID;
  }
  return 0;
}

int
ioman_handle_readables(ioman_t man, fd_set *R_fds){
  int fd, stat;
  sock_t new_sock;
  channel_t chan, new_chan;
  channel_list_cell_t chan_cell, dead_cell;

  //printf("%d: ioman_handle_readables\n", man -> node_id);fflush(stdout);

  /* notify pipe for event */
  if(FD_ISSET(man -> pipe_R[0], R_fds) && ioman_handle_event(man) == IOMAN_EVENT_FINALIZE){
    return -1;
  }

  /* listen sock */
  fd = sock_fileno(man -> lsock);
  if(FD_ISSET(fd, R_fds)){
    new_sock = sock_accept(man -> lsock);

    /* add channel to list */
    new_chan = channel_active_create(new_sock);
    channel_list_append(man -> channels, new_chan);

    man -> fdset_valid = IOMAN_FDSET_CACHE_INVALID;
  }

  /* local chan */
  fd = sock_fileno(channel_get_sock(man -> local_chan));
  if(FD_ISSET(fd, R_fds)){
    stat =ioman_process_local_channel_read(man, man -> local_chan);
    assert(stat == 0);
  }

  /* handle active channels */
  for(chan_cell = channel_list_head(man -> channels); chan_cell != channel_list_end(man -> channels);){
    chan = channel_list_cell_data(chan_cell);
    fd = sock_fileno(channel_get_sock(chan));

    if(FD_ISSET(fd, R_fds)){
      //printf("%d: ioman_handle_readables fd: %d \n", man -> node_id, fd);fflush(stdout);
      if(ioman_process_channel_read(man, chan) != 0){
	/* if channel is dead, remove channel */
	dead_cell = chan_cell;
	chan_cell = channel_list_cell_next(dead_cell);
	
	channel_list_remove(man -> channels, dead_cell);
/* 	printf("%d: error in channel_read :%d\n", man -> node_id, chan -> peer_id);fflush(stdout); */
	ioman_delete_channel(man, chan);
	continue;
      }
    }
    
    /* next */
    chan_cell = channel_list_cell_next(chan_cell);
  }

  /* normal return */
  return 0;
}

int
ioman_handle_writables(ioman_t man, fd_set *W_fds){
  int fd;
  channel_t chan;
  channel_list_cell_t chan_cell, dead_cell;

  //printf("%d: ioman_handle_writables\n", man -> node_id);fflush(stdout);

  /* handle active channels */
  for(chan_cell = channel_list_head(man -> channels); chan_cell != channel_list_end(man -> channels);){
    chan = channel_list_cell_data(chan_cell);
    fd = sock_fileno(channel_get_sock(chan));

    if(FD_ISSET(fd, W_fds)){
      //printf("%d: ioman_handle_writables fd: %d \n", man -> node_id, fd);fflush(stdout);
      if(ioman_process_channel_write(man, chan) != 0){
	/* if channel is dead, remove channel */
	dead_cell = chan_cell;
	chan_cell = channel_list_cell_next(dead_cell);
	
	channel_list_remove(man -> channels, dead_cell);
/* 	printf("%d: error in channel_write :%d\n", man -> node_id, chan -> peer_id);fflush(stdout); */
	ioman_delete_channel(man, chan);
	continue;
      }
    }

    /* next */
    chan_cell = channel_list_cell_next(chan_cell);
  }

  /* normal return */
  return 0;
}

void
ioman_send_msg(ioman_t man, int kind, int dst_id, const void* buff, int len){
  channel_t chan = man -> channel_map[dst_id];
  assert(chan != NULL);
  ioman_sys_send_msg(man, chan, kind, dst_id, buff, len);
  /* notify io thread */
  ioman_notify_newmsg(man);
}

void
ioman_bcast_msg(ioman_t man, int kind, const void* buff, int len){
  int e = IOMAN_EVENT_BCASTMSG;

  /* take copy of message */
  void* local_buff = std_malloc(len);
  std_memcpy(local_buff, buff, len);

  std_pthread_mutex_lock(&man -> lock);
  std_write(man -> pipe_R[1], &e, sizeof(int));

  std_write(man -> pipe_R[1], &kind, sizeof(int));
  std_write(man -> pipe_R[1], &local_buff, sizeof(void*));
  std_write(man -> pipe_R[1], &len, sizeof(int));
  std_pthread_mutex_unlock(&man -> lock);
}

void
ioman_send_chunk(ioman_t man, int dst_id, sid_t sid, int tot_len, int seq, const void* buff, int len){
  msg_info_t minfo = msg_info_create();

  minfo -> kind   = MSG_TYPE_DATA;
  minfo -> dst_id = dst_id;
  minfo -> src_id = man -> node_id;
  minfo -> len    = len;

  /* data chunk specific fields */
  minfo -> tot_len    = tot_len;
  minfo -> sid    = sid;
  minfo -> seq    = seq;

  /* push to local channel */
  channel_local_push_chunk(man -> local_chan, minfo, buff, len);

  /* notify io thread */
  ioman_notify_newmsg(man);
  
  msg_info_destroy(minfo);
}

void
ioman_print_raw_set_fds(ioman_t man, int maxfd, fd_set *fds){
  int i;
  printf("fds [");
  for(i = 0; i < maxfd; i++){
    if(FD_ISSET(i, fds))
      printf("%d, ", i);
  }
  printf("]\n");
}

void*
ioman_handler_loop(void* _man){
  ioman_t man = (ioman_t)_man;

  fd_set R_fds, W_fds;
  int maxfd;
  int nready;

  while(1){
    ioman_set_fds(man, &R_fds, &W_fds, &maxfd);

    //printf("%d: select ... maxfd %d\n", man -> node_id, maxfd);fflush(stdout);
    //printf("%d: waiting: \n", man -> node_id);fflush(stdout);
    //printf("%d: R: ", man -> node_id); ioman_check_set_fds(man, &R_fds);fflush(stdout);
    //printf("%d: W: ", man -> node_id); ioman_check_set_fds(man, &W_fds);fflush(stdout);
    nready = std_select(maxfd + 1, &R_fds, &W_fds, NULL, NULL);

    //ioman_print_raw_set_fds(man, maxfd + 1, &R_fds);
    /* printf("%d: loop... ready %d maxfd %d\n", man -> node_id, nready, maxfd);fflush(stdout); */
    //printf("%d: readable : ", man -> node_id); ioman_check_set_fds(man, &R_fds);fflush(stdout);
    //printf("%d: writable : ", man -> node_id); ioman_check_set_fds(man, &W_fds);
    if(ioman_handle_readables(man, &R_fds) == -1)break;
    if(ioman_handle_writables(man, &W_fds) == -1)break;
  }

  ioman_finalize(man);

  //printf("ioman_handler_loop end...\n");fflush(stdout);
  
  return NULL;
}

void
ioman_start(ioman_t man){
  man -> lsock = listen_sock_create(0, 128);
  std_pthread_create(&man -> handler, NULL, ioman_handler_loop, (void*)man);
}
