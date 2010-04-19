#ifndef __IMPL_COMM_H__
#define __IMPL_COMM_H__

#include <comm/comm.h>

#define COMM_MAX_PEER (1024)               // hard coded max number of node communicators
#define COMM_HASH_SIZE (128)               // size of the hash bucket
#define COMM_DATA_CHUNK_SIZE (1024 * 1024) // chunk size to which messages will be fragmented for sending
#define COMM_ADJUST_CHUNK_SIZE (0)         // set to 1, to adjust chunk size based on bandwidth with destination 

#define COMM_MONITOR_RECV_BAND (0)         // set to 1, to run a thread that monitors send/recv bandwidth for node communicators

#include "ioman.h"
#include <struct/rtable.h>

#include <pthread.h>

struct comm_node {
  int node_id;
  ioman_t man;

  channel_hash_map_t pending_conns;

  pthread_mutex_t lock;
  pthread_cond_t cond;
  
  double rtts[COMM_MAX_PEER];
  overlay_rtable_t rts[COMM_MAX_PEER];
  int num_rts;

  int data_msg_chunk_size;
  sid_t data_msg_sid;

  /* num. of data msgs received */
  int data_msg_recvd;
  /* num. of undelivered data msgs received */
  /* mostly used for comm_node_recv_any_data() */
  int data_msg_unacked_recvd;

  /* to decide msg chunk size */
  int max_width;
  int min_width;

  /* TODO: temporary */
  data_msg_hash_map_t data_msg_map;
  data_msg_list_t* recvd_data_msg_queues; // sid -> recvd_data_msg_queue

  /* for stats */
  long recvd_bytes;

#if COMM_MONITOR_RECV_BAND
  int monitor_run;
  pthread_t monitor;
#endif
  
};

void comm_node_exchange_rt(comm_node_t node, overlay_rtable_t rt, int num_peers);

void comm_node_notify_connect(comm_node_t node, channel_t chan);
void comm_node_notify_success(comm_node_t node, channel_t chan);
void comm_node_notify_failure(comm_node_t node, channel_t chan);
int comm_node_lookup_rt(comm_node_t node, int src_pid, int dst_pid);
void comm_node_handle_msg(comm_node_t node, channel_t chan, const msg_info_t msg_info, const msg_buff_t buff);
void comm_node_setup_chunk(comm_node_t node, channel_t chan, const msg_info_t header);
void comm_node_handle_chunk(comm_node_t node, channel_t chan, const msg_info_t header, const msg_buff_t chunk);


#endif // __IMPL_COMM_H__
