#ifndef __GRAPH_OVERLAY_H__
#define __GRAPH_OVERLAY_H__

#include <std/vector.h>
#include <std/list.h>
#include <xml/topology.h>

typedef struct overlay_node overlay_node, *overlay_node_t;

typedef struct overlay_edge {
  /* weak references to nodes */
  overlay_node_t n0;
  overlay_node_t n1;

  float len;
  float width;
} overlay_edge, *overlay_edge_t;
VECTOR_MAKE_TYPE_INTERFACE(overlay_edge)

/* comparators for sorting edges */
int overlay_edge_sort_by_width(overlay_edge_t e0, overlay_edge_t e1);
int overlay_edge_sort_by_length(overlay_edge_t e0, overlay_edge_t e1);
int overlay_edge_sort_by_degree(overlay_edge_t e0, overlay_edge_t e1);

struct overlay_node {
  int pid;
  char* name;
  overlay_edge_vector_t conns;/* contents are weak refs */
};
VECTOR_MAKE_TYPE_INTERFACE(overlay_node)
LIST_MAKE_TYPE_INTERFACE(overlay_node)



typedef struct overlay_graph {
  overlay_edge_vector_t conns;
  overlay_node_vector_t hosts;
} overlay_graph, *overlay_graph_t;

overlay_graph_t overlay_graph_create();
void overlay_graph_destroy(overlay_graph_t graph);
overlay_node_t overlay_graph_get_node(overlay_graph_t graph, const char* hostname);
overlay_node_t overlay_graph_get_node_by_pid(overlay_graph_t graph, int pid);
overlay_node_t overlay_graph_add_node(overlay_graph_t graph, int pid, const char* hostname);
overlay_edge_t  overlay_graph_add_conn(overlay_graph_t graph, xml_topology_t xml_top, overlay_node_t src, overlay_node_t dst);
overlay_edge_t overlay_graph_add_conn_by_pid(overlay_graph_t graph, xml_topology_t xml_top, int src_pid, int dst_pid);
void overlay_graph_simulate_random(overlay_graph_t graph, xml_topology_t xml_top, float density, int seed);
void overlay_graph_simulate_ring(overlay_graph_t graph, xml_topology_t xml_top);
void overlay_graph_simulate_locality_aware(overlay_graph_t graph, xml_topology_t xml_top, int alpha, int seed);

#endif // __GRAPH_OVERLAY_H__
