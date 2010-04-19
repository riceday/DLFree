#ifndef __GRAPH_DIJKSTRA_H__
#define __GRAPH_DIJKSTRA_H__

#include <struct/rtable.h>

#define MAX_DIJKSTRA_DIST (10000000.0)
#define MAX_DIJKSTRA_WIDTH (1000000000.0)

typedef struct leveled_dijkstra{
  overlay_node_t* pid_to_node; // [pid] -> node : these are weak refs
  float** dist; // [pid][level] -> dist

  int** prev; // [pid][level] -> pid
  int** prev_level; // [pid][level] -> level

  int** levels; // [pid][pid] -> level
  float** metrics; // [pid][pid] -> value
  float** width; // [pid][pid] -> value

  int** reached; // [pid][level] -> 0 or 1
  
  int num_nodes;
  int num_levels;
  
} leveled_dijkstra, *leveled_dijkstra_t;

leveled_dijkstra_t
leveled_dijkstra_create(overlay_node_vector_t nodes, int num_levels);
void
leveled_dijkstra_destroy(leveled_dijkstra_t dijk);
overlay_rtable_t
leveled_dijkstra_run(leveled_dijkstra_t dijk, router_graph_link_list_t links, overlay_node_t srcnode, int seed);



#endif // __GRAPH_DIJKSTRA_H__
