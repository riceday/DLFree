#ifndef __XML_TOPOLOGY_H__
#define __XML_TOPOLOGY_H__
#include <std/vector.h>

#define XML_TOPOLOGY_RANDOMIZE_RANGE (0.1)

typedef struct xml_top_node xml_top_node, *xml_top_node_t;

typedef struct xml_top_edge{
  /* weak references */
  xml_top_node_t n0;
  xml_top_node_t n1;

  float len;
  float width;
  
} xml_top_edge, *xml_top_edge_t;

VECTOR_MAKE_TYPE_INTERFACE(xml_top_edge)
     
struct xml_top_node{
  char* name;
  xml_top_node_t parent;
  xml_top_edge_vector_t edges; /* contents are weak refs */
};
void xml_top_node_print(xml_top_node_t node);

VECTOR_MAKE_TYPE_INTERFACE(xml_top_node)

typedef struct xml_topology{
  xml_top_node_vector_t nodes;
  xml_top_edge_vector_t edges;
} xml_topology, *xml_topology_t;

xml_topology_t xml_topology_create();
void xml_topology_destroy(xml_topology_t top);
xml_top_node_t xml_topology_add_node(xml_topology_t top, const char* nodename, xml_top_node_t parent);
xml_top_node_t xml_topology_add_edge(xml_topology_t top, const xml_top_node_t parent, const char* nodename, float len, float width);
void xml_topology_randomize_edge_width(xml_topology_t top, int seed);
int xml_topology_traverse(xml_topology_t top, const char* srcname, const char* dstname, xml_top_node_vector_t path, float *len, float *width);
void xml_topology_print(xml_topology_t top, xml_top_node_t node, int depth);


#endif // __XML_TOPOLOGY_H__
