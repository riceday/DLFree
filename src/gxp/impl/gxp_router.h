#ifndef __IMPL_GXP_ROUTER_H__
#define __IMPL_GXP_ROUTER_H__

#include <xml/topology.h>
#include <graph/overlay.h>
#include <struct/rtable.h>
#include "gxp.h"

typedef struct gxp_router {
  xml_topology_t xml_top;
  overlay_graph_t graph;
} gxp_router, *gxp_router_t;

gxp_router_t gxp_router_create(const char* filename, int seed);
void gxp_router_destroy(gxp_router_t router);
overlay_rtable_t gxp_router_run(gxp_router_t router, gxp_man_t gxpman, const int** conn_mat, int rttype, int seed);

#endif // __IMPL_GXP_ROUTER_H__
