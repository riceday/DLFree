#ifndef __GRAPH_ROUTER_H__
#define __GRAPH_ROUTER_H__

#include "overlay.h"
#include <std/list.h>
#include <std/vector.h>
#include <std/uset.h>

enum ordered_link_router_type{
  ORDERED_LINK_ROUTER_RANDOM,
  ORDERED_LINK_ROUTER_BAND,
  ORDERED_LINK_ROUTER_HOPS,
  ORDERED_LINK_ROUTER_HUB,
};

enum updown_router_type{
  UPDOWN_ROUTER_BFS,
  UPDOWN_ROUTER_DFS,
};

typedef struct router_graph_link {
  overlay_edge_t conn; /* weak reference */

  overlay_node_t up;
  overlay_node_t down;
  int level_0; /* level for n0 -> n1 edge */
  int level_1; /* level for n1 -> n0 edge */
  
} router_graph_link, *router_graph_link_t;

router_graph_link_t
router_graph_link_create(overlay_edge_t conn);
void
router_graph_link_destroy(router_graph_link_t link);

LIST_MAKE_TYPE_INTERFACE(router_graph_link)
VECTOR_MAKE_TYPE_INTERFACE(router_graph_link)
UNIONSET_MAKE_TYPE_INTERFACE(overlay_node)

router_graph_link_list_t
router_graph_make_spanning_trees(overlay_graph_t graph, int *level, int type, int seed);
router_graph_link_list_t
router_graph_make_updown_tree(overlay_graph_t graph, int type, int seed);

typedef struct router_bfs_node {
  int pid;
  float value;

  router_graph_link_vector_t links;
  
} router_bfs_node, *router_bfs_node_t;
LIST_MAKE_TYPE_INTERFACE(router_bfs_node)
VECTOR_MAKE_TYPE_INTERFACE(router_bfs_node)

router_bfs_node_t router_bfs_node_create(int pid, float val);
void router_bfs_node_destroy(router_bfs_node_t bfs_node);

router_graph_link_list_t
router_graph_make_bfs_spanning_trees(overlay_graph_t graph, int nnodes, float* avgdist_map, int *level, int seed);


/* WARNING: only use below if you want to get deadlocking overlay */
router_graph_link_list_t
router_graph_make_deadlock_prone_links(overlay_graph_t graph);
  
#endif // __GRAPH_ROUTER_H__
