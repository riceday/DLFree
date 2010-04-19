#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <assert.h>

#include <std/std.h>
#include "xml/topology.h"

VECTOR_MAKE_TYPE_IMPLEMENTATION(xml_top_edge)
VECTOR_MAKE_TYPE_IMPLEMENTATION(xml_top_node)

static float
randomize_metric(float w, float r){
  float min_r = 1.0 - r;
  float ratio = ((float)rand() / RAND_MAX) * (2 * r) + min_r;
  return w * ratio;
}

xml_top_edge_t
xml_top_edge_create(const xml_top_node_t n0, const xml_top_node_t n1, float len, float width){
  xml_top_edge_t e = (xml_top_edge_t) std_malloc(sizeof(xml_top_edge));
  e -> n0 = n0;
  e -> n1 = n1;
  e -> len = len;
  e -> width = width;

  return e;
}

void
xml_top_edge_destroy(xml_top_edge_t e){
  /* do not free e -> {n0, n1} because weak ref */
  std_free(e);
}
     
xml_top_node_t
xml_top_node_create(const char* nodename, xml_top_node_t parent){
  xml_top_node_t node = (xml_top_node_t)std_malloc(sizeof(xml_top_node));
  int name_len = strlen(nodename);
  
  node -> name = std_malloc(sizeof(char) * (name_len + 1));
  std_memcpy(node -> name, nodename, sizeof(char) * (name_len + 1));

  node -> parent = parent;
  node -> edges = xml_top_edge_vector_create(2);

  return node;
}

void
xml_top_node_destroy(xml_top_node_t node){
  std_free(node -> name);
  xml_top_edge_vector_destroy(node -> edges);  /* contents are weak refs */
  std_free(node);
}

void
xml_top_node_print(xml_top_node_t node){
  printf("node(%s)", node -> name);
}

void
xml_top_node_add_edge(xml_top_node_t node, xml_top_edge_t e){
  xml_top_edge_vector_add(node -> edges, e);
}

xml_topology_t
xml_topology_create(){
  xml_topology_t top = (xml_topology_t)std_malloc(sizeof(xml_topology));
  top -> nodes = xml_top_node_vector_create(1);
  top -> edges = xml_top_edge_vector_create(1);

  return top;
}

void
xml_topology_destroy(xml_topology_t top){
  xml_top_node_t node;
  xml_top_edge_t edge;

  while(xml_top_node_vector_size(top -> nodes)){
    node = xml_top_node_vector_pop(top -> nodes);
    xml_top_node_destroy(node);
  }
  while(xml_top_edge_vector_size(top -> edges)){
    edge = xml_top_edge_vector_pop(top -> edges);
    xml_top_edge_destroy(edge);
  }

  xml_top_node_vector_destroy(top -> nodes);
  xml_top_edge_vector_destroy(top -> edges);

  std_free(top);
}

xml_top_node_t
xml_topology_add_node(xml_topology_t top, const char* nodename, xml_top_node_t parent){
  xml_top_node_t new_node = xml_top_node_create(nodename, parent);
  xml_top_node_vector_add(top -> nodes, new_node);

  return new_node;
}

xml_top_node_t
xml_topology_get_node(xml_topology_t top, const char* nodename){
  xml_top_node_t node;
  int i;
  for(i = 0; i < xml_top_node_vector_size(top -> nodes); i++){
    node = xml_top_node_vector_get(top -> nodes, i);
    
    if(strcmp(node -> name, nodename) == 0) /* match */
      return node;
  }
  return NULL;
}

xml_top_node_t
xml_topology_add_edge(xml_topology_t top, const xml_top_node_t parent, const char* nodename, float len, float width){
  xml_top_node_t new_node = xml_top_node_create(nodename, parent);
  xml_top_edge_t new_edge = xml_top_edge_create(parent, new_node, len, width);

  /* add edge to each other's node */
  xml_top_node_add_edge(parent, new_edge);
  xml_top_node_add_edge(new_node, new_edge);

  /* add new node and edge to topology */
  xml_top_node_vector_add(top -> nodes, new_node);
  xml_top_edge_vector_add(top -> edges, new_edge);

  return new_node;
}

void
xml_topology_randomize_edge_width(xml_topology_t top, int seed){
  xml_top_edge_t e;
  int i;
  srand(seed);

  for(i = 0; i < xml_top_edge_vector_size(top -> edges); i++){
    e = xml_top_edge_vector_get(top -> edges, i);
    e -> width = randomize_metric(e -> width, XML_TOPOLOGY_RANDOMIZE_RANGE);
  }
}

int
xml_topology_traverse(xml_topology_t top, const char* srcname, const char* dstname, xml_top_node_vector_t path, float *len, float *width){
  xml_top_node_t node, child;
  xml_top_edge_t edge;
  float tmp_len, tmp_width;
  int i;

  assert(path != NULL);
  /* if first node, initialize values */
  if(xml_top_node_vector_size(path) == 0){
    node = xml_topology_get_node(top, srcname); assert(node != NULL);
    *len = 0.0;
    *width = FLT_MAX;
    xml_top_node_vector_add(path, node);
  }
  node = xml_top_node_vector_last(path);

  if(strcmp(node -> name, dstname) == 0) return 1; /* match */

  tmp_len = *len;
  tmp_width = *width;
  for(i = 0; i < xml_top_edge_vector_size(node -> edges); i++){
    edge = xml_top_edge_vector_get(node -> edges, i);
    
    /* figure out which end is self */
    /* need to traverse the other end */
    if(strcmp(node -> name, edge -> n0 -> name) == 0)
      child = edge -> n1;
    else
      child = edge -> n0;

    /* do not traverse if node in path */
    if(xml_top_node_vector_contains(path, child))
      continue;

    xml_top_node_vector_add(path, child);
    *len = tmp_len + edge -> len;
    *width = tmp_width > edge -> width ? edge -> width : tmp_width; /* take MIN */
    if(xml_topology_traverse(top, srcname, dstname, path, len, width)) /* match downstream */
      return 1;
    xml_top_node_vector_pop(path); /* backtrack */
  }
  
  /* not found */
  return 0;
}

void
xml_topology_print(xml_topology_t top, xml_top_node_t node, int depth){
  xml_top_edge_t edge;
  xml_top_node_t child;
  int i;

  if(node == NULL && xml_top_node_vector_size(top -> nodes)){
    node = xml_top_node_vector_get(top -> nodes, 0);
    depth = 0;
  }

  if(node != NULL){

    for(i = 0; i < depth; i++)printf("  ");
    printf("[%s(%d)]\n", node -> name, depth);

    for(i = 0; i < xml_top_edge_vector_size(node -> edges); i++){
      edge = xml_top_edge_vector_get(node -> edges, i);

      /* figure out which end is self */
      /* need to traverse the other end */
      if(strcmp(node -> name, edge -> n0 -> name) == 0)
	child = edge -> n1;
      else
	child = edge -> n0;

      /* do not go up parent on traversal */
      if(node -> parent == child)
	 continue;
      
      xml_topology_print(top, child, depth + 1);
    }
  }

}

