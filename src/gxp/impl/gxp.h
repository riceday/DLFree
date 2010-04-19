#ifndef __IMPL_GXP_H__
#define __IMPL_GXP_H__

// extinc
#include <dlfree/gxp.h>

#include <iface/ep.h>

#define GXP_NO_RTT (-1)
#define GXP_CLUSTER_NAME_PREFIX (5)

enum gxp_connect_status{
  GXP_CONNECT_NO,
  GXP_CONNECT_YES,
  GXP_CONNECT_FAIL,
};

struct gxp_man {
  int gxp_idx;
  int gxp_num_execs;
  char* gxp_hostname;
  
  FILE* rfp;
  FILE* wfp;

  char** peer_hostnames;

  inet_ep_t* peer_eps;

  int **conn_mat; /* mat[src_idx][dst_idx] == 1 if connected */

  /* to specify preferrence for endpoint inet address to declare */
  const char **ep_prio_pats;
  int ep_prio_count;
  
};


#endif // __IMPL_GXP_H__
