#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <std/std.h>
#include <xml/topology.h>
#include <xml/parser.h>
#include "graph/overlay.h"

VECTOR_MAKE_TYPE_IMPLEMENTATION(overlay_node)
VECTOR_MAKE_TYPE_IMPLEMENTATION(overlay_edge)

LIST_MAKE_TYPE_IMPLEMENTATION(overlay_node)
     
overlay_node_t
overlay_node_create(int pid, const char* name){
  overlay_node_t node = (overlay_node_t) std_malloc(sizeof(overlay_node));
  int name_len = strlen(name);
  node -> name = (char*) std_malloc(sizeof(char) * (name_len + 1));
  std_memcpy(node -> name, name, sizeof(char) * (name_len + 1));

  node -> pid = pid;

  node -> conns = overlay_edge_vector_create(1);
  
  return node;
}

void
overlay_node_destroy(overlay_node_t node){
  std_free(node -> name);
  overlay_edge_vector_destroy(node -> conns);/* contents are weak refs */
  std_free(node);
}

overlay_edge_t
overlay_edge_create(overlay_node_t n0, overlay_node_t n1, float len, float width){
  overlay_edge_t edge = (overlay_edge_t) std_malloc(sizeof(overlay_edge));

  edge -> n0 = n0;
  edge -> n1 = n1;
  edge -> len = len;
  edge -> width = width;

  return edge;
}

void
overlay_edge_destroy(overlay_edge_t edge){
  /* we do not free edge -> {n0, n1} because it is a weak ref */
  std_free(edge);
}

/* comparator for edge sorter: wider edge comes first */
int
overlay_edge_sort_by_width(overlay_edge_t e0, overlay_edge_t e1){
  if(e0 -> width > e1 -> width)
    return -1;
  else if(e0 -> width == e1 -> width)
    return 0;
  else
    return 1;
}

/* comparator for edge sorter: shorter edge comes first */
int
overlay_edge_sort_by_length(overlay_edge_t e0, overlay_edge_t e1){
  if(e0 -> len < e1 -> len)
    return -1;
  else if(e0 -> len == e1 -> len)
    return 0;
  else
    return 1;
}

/* comparator for edge sorter: edge with more hub like nodes comes first */
int
overlay_edge_sort_by_degree(overlay_edge_t e0, overlay_edge_t e1){
  int c0 = overlay_edge_vector_size(e0 -> n0 -> conns) + overlay_edge_vector_size(e0 -> n1 -> conns);
  int c1 = overlay_edge_vector_size(e1 -> n0 -> conns) + overlay_edge_vector_size(e1 -> n1 -> conns);
  if(c0 < c1)
    return -1;
  else if(c0 == c1)
    return 0;
  else
    return 1;
}

overlay_graph_t
overlay_graph_create(){
  overlay_graph_t graph = (overlay_graph_t) std_malloc(sizeof(overlay_graph));

  graph -> conns = overlay_edge_vector_create(1);
  graph -> hosts = overlay_node_vector_create(1);

  return graph;
}

void
overlay_graph_destroy(overlay_graph_t graph){
  overlay_node_t node;
  overlay_edge_t edge;
  while(overlay_node_vector_size(graph -> hosts)){
    node = overlay_node_vector_pop(graph -> hosts);
    overlay_node_destroy(node);
  }
  while(overlay_edge_vector_size(graph -> conns)){
    edge = overlay_edge_vector_pop(graph -> conns);
    overlay_edge_destroy(edge);
  }

  overlay_edge_vector_destroy(graph -> conns);
  overlay_node_vector_destroy(graph -> hosts);

  std_free(graph);
}

overlay_node_t
overlay_graph_get_node(overlay_graph_t graph, const char* hostname){
  int i;
  overlay_node_t node = NULL;
  /*  TODO: this linear search is slow */
  for(i = 0; i < overlay_node_vector_size(graph -> hosts); i ++){
    node = overlay_node_vector_get(graph -> hosts, i);
    if(strcmp(node -> name, hostname) == 0){ /* match */
      return node;
    }
  }

  return NULL;
}

overlay_node_t
overlay_graph_get_node_by_pid(overlay_graph_t graph, int pid){
  int i;
  overlay_node_t node = NULL;
  /*  TODO: this linear search is slow */
  for(i = 0; i < overlay_node_vector_size(graph -> hosts); i ++){
    node = overlay_node_vector_get(graph -> hosts, i);
    if(node -> pid == pid){ /* match */
      return node;
    }
  }

  return NULL;
}

overlay_node_t
overlay_graph_add_node(overlay_graph_t graph, int pid, const char *hostname){
  overlay_node_t node = overlay_node_create(pid, hostname);
  overlay_node_vector_add(graph -> hosts, node);
  return node;
}

overlay_edge_t
overlay_graph_add_conn(overlay_graph_t graph, xml_topology_t xml_top, overlay_node_t src, overlay_node_t dst){
  float len, width;
  xml_top_node_vector_t path = xml_top_node_vector_create(1);
  overlay_edge_t e;
  int found = xml_topology_traverse(xml_top, src -> name, dst -> name, path, &len, &width);
  assert(found); /* path from srcname to dstname is on overlay path */

/*   printf("%s -> %s %.3f %.3f\n", src -> name, dst -> name, len, width); */
  e = overlay_edge_create(src, dst, len, width);
  overlay_edge_vector_add(src -> conns, e);
  overlay_edge_vector_add(dst -> conns, e);
  
  overlay_edge_vector_add(graph -> conns, e);

  xml_top_node_vector_destroy(path);

  return e;
}

overlay_edge_t
overlay_graph_add_conn_by_pid(overlay_graph_t graph, xml_topology_t xml_top, int src_pid, int dst_pid){
  overlay_node_t src = overlay_graph_get_node_by_pid(graph, src_pid);
  overlay_node_t dst = overlay_graph_get_node_by_pid(graph, dst_pid);
  assert(src != NULL); /* make sure that src/dst nodes have already been added */
  assert(dst != NULL);
  return overlay_graph_add_conn(graph, xml_top, src, dst);
}

static int
overlay_graph_simulate_add_nodes(overlay_graph_t graph, xml_topology_t xml_top){
  int pid, i;
  xml_top_node_vector_t xml_nodes = xml_top -> nodes;
  xml_top_node_t xml_src;

  for(pid = 0, i = 0; i < xml_top_node_vector_size(xml_nodes); i++){
    xml_src = xml_top_node_vector_get(xml_nodes, i);
    if(strncmp(xml_src -> name, XML_SWITCH_NAME_PREFIX, strlen(XML_SWITCH_NAME_PREFIX)) == 0)
       continue;
    else if(strncmp(xml_src -> name, XML_ROOT_NAME_PREFIX, strlen(XML_ROOT_NAME_PREFIX)) == 0)
      continue;
    overlay_graph_add_node(graph, pid, xml_src -> name);
    pid ++;
  }
    
  return overlay_node_vector_size(graph -> hosts);
}

void
overlay_graph_simulate_random(overlay_graph_t graph, xml_topology_t xml_top, float density, int seed){
  int i, j;
  overlay_node_t src, dst;
  int pairs, conns;

  overlay_graph_simulate_add_nodes(graph, xml_top);
    
  srand(seed);
  conns = 0; pairs = 0;
  for(i = 0; i < overlay_node_vector_size(graph -> hosts); i++){
    src = overlay_node_vector_get(graph -> hosts, i);
    for(j = i + 1; j < overlay_node_vector_size(graph -> hosts); j++){
      dst = overlay_node_vector_get(graph -> hosts, j);

      if((float)rand() / RAND_MAX < density){
	overlay_graph_add_conn(graph, xml_top, src, dst);
	conns ++;
      }
      pairs ++;
    }
  }
  printf("established %d/%d connections density: %.3f\n", conns, pairs, ((float)conns) / pairs);
}

void
overlay_graph_simulate_ring(overlay_graph_t graph, xml_topology_t xml_top){
  int i;
  overlay_node_t src, dst;

  overlay_graph_simulate_add_nodes(graph, xml_top);

  for(i = 0; i < overlay_node_vector_size(graph -> hosts); i++){
    src = overlay_node_vector_get(graph -> hosts, i);
    dst = overlay_node_vector_get(graph -> hosts, (i + 1) % (overlay_node_vector_size(graph -> hosts)));
    overlay_graph_add_conn(graph, xml_top, src, dst);
  }
}

#ifndef Min
#define Min(x, y) ((x) > (y) ? (y) : (x))
#endif

static void
shuffle_array(int *a, int n){
  int i, j, tmp;
  for(i = 0; i < n; i++){
    j = rand() % n;
    tmp = a[i]; a[i] = a[j]; a[j] = tmp;
  }
}

static void
shuffle_and_sort_pairs(float* k, int* v, int n){
  int i, j;
  float tmpk;
  int tmpv;

  /* first shuffle */
  for(i = 0; i < n; i++){
    j = rand() % n;
    tmpk = k[i];k[i] = k[j];k[j] = tmpk;
    tmpv = v[i];v[i] = v[j];v[j] = tmpv;
  }

  /* bubble sort */
  for(i = 0; i < n; i++){
    for(j = 0; j < n - 1; j++){
      if(k[j] > k[j + 1]){
	tmpk = k[j];k[j] = k[j+1];k[j+1] = tmpk;
	tmpv = v[j];v[j] = v[j+1];v[j+1] = tmpv;
      }
    }
  }
}

void
overlay_graph_simulate_locality_aware(overlay_graph_t graph, xml_topology_t xml_top, int alpha, int seed){
  int i, j, k, lim, pairs, conns, *idxs, **conn_plan;
  int src_idx, dst_idx, nnodes, npeers;
  overlay_node_t src, dst;
  float len, width, *dist;
  xml_top_node_vector_t path = xml_top_node_vector_create(1);

  assert(alpha > 0);

  nnodes = overlay_graph_simulate_add_nodes(graph, xml_top);
  npeers = nnodes - 1;

  dist = (float*)std_calloc(npeers, sizeof(float));
  idxs = (int*)std_calloc(npeers, sizeof(int));
  conn_plan = (int**)std_calloc(nnodes, sizeof(int*));
  for(i = 0; i < nnodes;i ++)
    conn_plan[i] = (int*)std_calloc(nnodes, sizeof(int));

  conns = 0; pairs = 0;
  for(src_idx = 0; src_idx < nnodes; src_idx++){
    /* figure out distance to all nodes */
    for(dst_idx = 0, i = 0; dst_idx < nnodes; dst_idx ++){
      if(dst_idx == src_idx){
	continue;
      }
      else{
	src = overlay_node_vector_get(graph -> hosts, src_idx);
	dst = overlay_node_vector_get(graph -> hosts, dst_idx);
	xml_topology_traverse(xml_top, src -> name, dst -> name, path, &len, &width);
	xml_top_node_vector_clear(path);
      }
      dist[i] = len;
      idxs[i] = dst_idx;
      i++;
    }

    srand(seed + src_idx);

    /* bubble sort: according to shorter dist */
    /* but first need to randomize to remove any bias */
    shuffle_and_sort_pairs(dist, idxs, npeers);
    
    /* decide to whom to connect according to distance */
    for(i = 0, lim = alpha; i < npeers; lim *= 2){
      j = Min(i + lim, npeers);
      /* randomize */
      shuffle_array(idxs + i, j - i);
      /* in the range of [i, j) try to select alpha nodes */
      /* boundary condition: when range of [i, j) < lim */
      for(k = i; k < j; k ++){
	if( lim == (j - i) && k >= i + alpha) break;
	if( lim == (j - i) || (float)rand() / RAND_MAX < (float)alpha / lim){
	  dst_idx = idxs[k];
	  conn_plan[src_idx][dst_idx] = 1;
	  conn_plan[dst_idx][src_idx] = 1;
	}
      }
      i = j;
    }
  }

  /* actually connect */
  for(src_idx = 0; src_idx < nnodes; src_idx++){
    for(dst_idx = src_idx + 1; dst_idx < nnodes; dst_idx++){
      if(conn_plan[src_idx][dst_idx]){
	src = overlay_node_vector_get(graph -> hosts, src_idx);
	dst = overlay_node_vector_get(graph -> hosts, dst_idx);
	overlay_graph_add_conn(graph, xml_top, src, dst);
	conns ++;
      }
      pairs ++;
    }
  }
/*   for(i = 0; i < overlay_node_vector_size(graph -> hosts); i++){ */
/*     src = overlay_node_vector_get(graph -> hosts, i); */
/*     printf("node: %s has %d conns\n", src -> name, overlay_edge_vector_size(src -> conns)); */
/*   } */
  printf("established %d/%d connections density: %.3f\n", conns, pairs, ((float)conns) / pairs);

  /* clean up */
  xml_top_node_vector_destroy(path);
  std_free(dist);
  std_free(idxs);
  for(i = 0; i < nnodes; i++)
    std_free(conn_plan[i]);
  std_free(conn_plan);
}
