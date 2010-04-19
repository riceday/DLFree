#ifndef __IMPL_IOMAN_H__
#define __IMPL_IOMAN_H__


//pre-declaration because of cross-reference
typedef struct ioman ioman, *ioman_t;

#include <pthread.h>
#include <std/std.h>
#include "sock.h"
#include "channel.h"
#include "comm.h"

enum ioman_event_type {
  IOMAN_EVENT_NEWCHAN,
  IOMAN_EVENT_NEWMSG,
  IOMAN_EVENT_BCASTMSG,
  IOMAN_EVENT_FINALIZE,
};

enum ioman_connect_status{
  IOMAN_CONNECT_OK,
  IOMAN_CONNECT_ERR,
};

enum ioman_fdset_cache_status{
  IOMAN_FDSET_CACHE_VALID,
  IOMAN_FDSET_CACHE_INVALID,
};

#define IOMAN_NOTIFYPIPE_BUFF_SIZE (1024 * 1024 * 4) /* 4MB */

struct ioman{
  int node_id;
  comm_node_t comm;
  char node_hostname[20];

  int pipe_R[2];
  
  channel_list_t channels;
  channel_t *channel_map;
  int maxpeers;

  channel_t local_chan;
  sock_t lsock;
  pthread_t handler;
  pthread_mutex_t lock;

  int maxfd;
  fd_set R_fds, W_fds;
  int fdset_valid;

/*   int use_cache; */
/*   int use_total; */
  
};

ioman_t ioman_create(int node_id, comm_node_t comm, int maxpeers);
void ioman_destroy(ioman_t ioman);

void ioman_register_channel(ioman_t man, int dst_id, channel_t chan);

int ioman_get_nsocks(ioman_t man);
void ioman_get_traffic_info(ioman_t man, long *rx_count, long *tx_count);
void ioman_get_send_buffer_info(ioman_t man, long *count);
void ioman_channel_tcp_info_print(ioman_t man, int dst_id);
void ioman_start(ioman_t man);
void ioman_stop(ioman_t man);
int ioman_listen_port(ioman_t man);
int ioman_new_connection(ioman_t man, const char* addr, int port, channel_t* chan);
void ioman_send_msg(ioman_t man, int kind, int dst_id, const void* buff, int len);
void ioman_bcast_msg(ioman_t man, int kind, const void* buff, int len);
void ioman_send_chunk(ioman_t man, int dst_id, sid_t sid, int tot_len, int seq, const void* buff, int len);

#endif // __IMPL_IOMAN_H__

