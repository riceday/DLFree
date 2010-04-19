#include "collective.h"
#include <dlfree/dlfree.h>
#include <dlfree/gxp.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <assert.h>

static double
timeval_diff(struct timeval *tv0, struct timeval *tv1){
  return (tv1->tv_sec - tv0->tv_sec) + (tv1->tv_usec - tv0->tv_usec) * 1e-6;
}

static void
wait_messages(dlfree_comm_node_t comm_node, int count){
  int src_id;
  void* buff;
  int buffsize;

  for(; count > 0; --count){
    dlfree_comm_node_recv_any_data(comm_node, &src_id, &buff, &buffsize);
    free(buff);
  }
}

/**
   This is for reduction. All nodes send the a message to a single node.
   This is a collective operation for all GXP nodes.

   \param man       GXP instance
   \param comm_node node instance
   \param dst_idx   GXP index of the node to send to
   \param len       length of the message
   \param iter      the number of iterations for the reduction
*/
void
send_all2one(gxp_man_t man, dlfree_comm_node_t comm_node, int dst_idx, long len, int iter){
  void *buff;
  int it;
  struct timeval tv0, tv2;
  double dt;

  int myidx = gxp_man_peer_id(man);
  int numpeers = gxp_man_num_peers(man);

  if(myidx == 0){
    printf("send_all2one: %ld [B] from dst_idx: %d\n", len, dst_idx);fflush(stdout);
  }

  buff = calloc(len, sizeof(char));
  sprintf(buff, "%d:%s says hello!", myidx, gxp_man_hostname(man));

  for(it = 0; it < iter; it++){
    gxp_man_sync(man);
    assert(gettimeofday(&tv0, NULL) == 0);

    // everyone except 1 node sends to 1 node
    if(myidx != dst_idx){
      dlfree_comm_node_send_data(comm_node, dst_idx, buff, len);
    }
    else{
      wait_messages(comm_node, numpeers - 1);
    }
    
    gxp_man_sync(man);
    assert(gettimeofday(&tv2, NULL) == 0);
    dt = timeval_diff(&tv0, &tv2);

    if(myidx == 0){
      fprintf(stdout, "send_all2one dst_idx: %d, %.3f,[MB/s], %.3f,[s]\n", dst_idx,
	      (float)len / (1000 * 1000) * (numpeers - 1) * (numpeers) / dt, dt);
      fflush(stdout);
    }
  }

  free(buff);
}

/**
   This is broadcast. One node sends a message to all other nodes.
   This is a collective operation for all GXP nodes.
   
   \param man       GXP instance
   \param comm_node node instance
   \param src_idx   GXP index of the node that will perform the broadcast
   \param len       length of the message
   \param iter      the number of iterations for the reduction
*/
void
send_one2all(gxp_man_t man, dlfree_comm_node_t comm_node, int src_idx, long len, int iter){
  void *buff;
  int it, idx;
  struct timeval tv0, tv2;
  double dt;

  int myidx = gxp_man_peer_id(man);
  int numpeers = gxp_man_num_peers(man);

  if(myidx == 0){
    printf("send_one2all: %ld [B] from src_idx: %d\n", len, src_idx);fflush(stdout);
  }

  buff = calloc(len, sizeof(char));
  sprintf(buff, "%d:%s says hello!", myidx, gxp_man_hostname(man));

  for(it = 0; it < iter; it++){
    gxp_man_sync(man);
    assert(gettimeofday(&tv0, NULL) == 0);

    // only 1 node sends to all
    if(myidx == src_idx){
      for(idx = 0; idx < numpeers; idx++){
	if(idx == myidx)
	  continue;
	
	dlfree_comm_node_send_data(comm_node, idx, buff, len);
      }
    }
    else{
      wait_messages(comm_node, 1);
    }
    
    gxp_man_sync(man);
    assert(gettimeofday(&tv2, NULL) == 0);
    dt = timeval_diff(&tv0, &tv2);

    if(myidx == 0){
      fprintf(stdout, "send_one2all src_idx: %d, %.3f,[MB/s], %.3f,[s]\n", src_idx,
	      (float)len / (1000 * 1000) * (numpeers - 1) * (numpeers) / dt, dt);
      fflush(stdout);
    }
  }

  free(buff);
}

/**
   This is for all-to-all broadcast. All nodes send the a message to all other nodes.
   This is a collective operation for all GXP nodes.

   \param man       GXP instance
   \param comm_node node instance
   \param len       length of the message
   \param iter      the number of iterations for the reduction
*/
void
send_all2all(gxp_man_t man, dlfree_comm_node_t comm_node, long len, int iter){
  void *buff;
  int i, it, idx;
  struct timeval tv0, tv2;
  double dt;
  int *ord;

  int myidx = gxp_man_peer_id(man);
  int numpeers = gxp_man_num_peers(man);

  if(myidx == 0){
    printf("send_all2all: %ld [B]\n", len);fflush(stdout);
  }

  buff = calloc(len, sizeof(char));
  sprintf(buff, "%d:%s says hello!", myidx, gxp_man_hostname(man));

  ord = (int*)malloc(sizeof(int) * numpeers);
  for(i = 0; i < numpeers; i++){
    ord[i] = (i + myidx) % numpeers;
  }

  for(it = 0; it < iter; it++){
    gxp_man_sync(man);
    assert(gettimeofday(&tv0, NULL) == 0);
    for(i = 0; i < numpeers; i++){
      idx = ord[i];
      if(idx == myidx)
	continue;
      
      dlfree_comm_node_send_data(comm_node, idx, buff, len);
    }
    
    wait_messages(comm_node, numpeers - 1);
    
    gxp_man_sync(man);
    assert(gettimeofday(&tv2, NULL) == 0);
    dt = timeval_diff(&tv0, &tv2);

    if(myidx == 0){
      fprintf(stdout, "send_all2all %d, %.3f,[MB/s], %.3f,[s]\n", numpeers,
	      (float)len / (1000 * 1000) * (numpeers - 1) * (numpeers) / dt, dt);
      fflush(stdout);
    }
  }

  free(buff);
  free(ord);
}
