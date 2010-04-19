#ifndef __DLFREE_GXP_H__
#define __DLFREE_GXP_H__

#include "dlfree.h"

#define GXP_WRITE_FD (3)
#define GXP_READ_FD (4)

// different routing algorithms
enum gxp_routing_type{
  GXP_RT_TYPE_UPDOWN_BFS,     // conventional up-down routing (breath-first)
  GXP_RT_TYPE_UPDOWN_DFS,     // proposed up-down routing     (depth-first)
  GXP_RT_TYPE_ORDERED_RANDOM, // order-link routing (choose random links)
  GXP_RT_TYPE_ORDERED_BAND,   // order-link routing (choose links with large bandwidth)
  GXP_RT_TYPE_ORDERED_HOPS,   // order-link routing (...)
  GXP_RT_TYPE_ORDERED_HUB,    // order-link routing (choose links connected to hub nodes first)
  GXP_RT_TYPE_ORDERED_BFS,    // order-link routing (choose links breath-first from 1 node)
  GXP_RT_TYPE_DEADLOCK,       // shortest path routing (deadlock-prone)
};

typedef struct gxp_man gxp_man, *gxp_man_t;

gxp_man_t gxp_man_create();
void gxp_man_destroy(gxp_man_t man);
void gxp_man_init(gxp_man_t man);
const char* gxp_man_hostname(gxp_man_t man);
int gxp_man_peer_id(gxp_man_t man);
int gxp_man_num_peers(gxp_man_t man);
void gxp_man_set_iface_prio(gxp_man_t man, const char **prio_pats, int npats);
void gxp_man_sync(gxp_man_t man);

const int** gxp_man_connect_all(gxp_man_t man, dlfree_comm_node_t comm);
const int** gxp_man_connect_random(gxp_man_t man, dlfree_comm_node_t comm, float p, int seed);
const int** gxp_man_connect_ring(gxp_man_t man, dlfree_comm_node_t comm);
const int** gxp_man_connect_line(gxp_man_t man, dlfree_comm_node_t comm);
const int** gxp_man_connect_with_gateway(gxp_man_t man, dlfree_comm_node_t comm, int prefix, int ngates, float p, int seed);
const int** gxp_man_connect_locality_aware(gxp_man_t man, dlfree_comm_node_t comm, const char* filename, int alpha, int seed);

void gxp_man_compute_rt(gxp_man_t man, dlfree_comm_node_t comm, const char* xml_filename, int rttype, int seed);
  
void gxp_man_ping_pong(gxp_man_t man, dlfree_comm_node_t comm, long len, int iter);

#endif // __DLFREE_GXP_H__
