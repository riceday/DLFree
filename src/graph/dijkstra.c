#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <std/std.h>
#include <std/long.h>
#include <struct/rtable.h>
#include "graph/router.h"
#include "graph/dijkstra.h"

static void
alloc_nodes(leveled_dijkstra_t dijk, overlay_node_vector_t nodes, int num_nodes, int num_levels){
  int i, l, pid;
  overlay_node_t node;

  overlay_node_t* pid_to_node = (overlay_node_t*)std_malloc(sizeof(overlay_node_t) * num_nodes);
  int** reached = (int**) std_malloc(sizeof(int*) * num_nodes);

  for(i = 0; i < overlay_node_vector_size(nodes); i++){
    node = overlay_node_vector_get(nodes, i);
    pid_to_node[node -> pid] = node;
  }

  for(pid = 0; pid < num_nodes; pid++){
    reached[pid] = (int*) std_malloc(sizeof(int) * num_levels);
    for(l = 0; l < num_levels; l++)
      reached[pid][l] = 0;
  }
  
  dijk -> pid_to_node = pid_to_node; 
  dijk -> reached = reached; 
}

static void
alloc_edges(leveled_dijkstra_t dijk, int num_nodes, int num_levels){
  int pid, l, p;
  /* final distance matrix */
  float** dist = (float**) std_malloc(sizeof(float*) * num_nodes);

  /* previous pointer */
  int** prev_level = (int**) std_malloc(sizeof(int*) * num_nodes);
  int** prev = (int**) std_malloc(sizeof(int*) * num_nodes);

  /* edge attributes */
  int** levels = (int**) std_malloc(sizeof(int*) * num_nodes);
  float** metrics = (float**) std_malloc(sizeof(float*) * num_nodes);
  float** width = (float**) std_malloc(sizeof(float*) * num_nodes);

  for(pid = 0; pid < num_nodes; pid++){
    dist[pid] = (float*) std_malloc(sizeof(float) * num_levels);

    prev_level[pid] = (int*) std_malloc(sizeof(int) * num_levels);
    prev[pid] = (int*) std_malloc(sizeof(int) * num_levels);

    levels[pid] = (int*) std_malloc(sizeof(int) * num_nodes);
    metrics[pid] = (float*) std_malloc(sizeof(float) * num_nodes);
    width[pid] = (float*) std_malloc(sizeof(float) * num_nodes);
    for(l = 0; l < num_levels; l++){
      dist[pid][l] = MAX_DIJKSTRA_DIST;

      prev_level[pid][l] = -1;
      prev[pid][l] = -1;
    }
    for(p = 0; p < num_nodes; p++){
      levels[pid][p] = -1;
      metrics[pid][p] = MAX_DIJKSTRA_DIST;
      width[pid][p] = 0;
    }
  }
  
  dijk -> dist = dist; 
  dijk -> prev_level = prev_level; 
  dijk -> prev = prev; 
  dijk -> levels = levels; 
  dijk -> metrics = metrics; 
  dijk -> width = width; 
}

leveled_dijkstra_t
leveled_dijkstra_create(overlay_node_vector_t nodes, int num_levels){
  leveled_dijkstra_t dijk = (leveled_dijkstra_t) std_malloc(sizeof(leveled_dijkstra));
  int num_nodes = overlay_node_vector_size(nodes);

  dijk -> num_nodes = num_nodes; 
  dijk -> num_levels = num_levels;

  alloc_nodes(dijk, nodes, num_nodes, num_levels);
  alloc_edges(dijk, num_nodes, num_levels);

  return dijk;
}

void
leveled_dijkstra_destroy(leveled_dijkstra_t dijk){
  int i;
  std_free(dijk -> pid_to_node);

  for(i = 0; i < dijk -> num_nodes; i ++){
    std_free(dijk -> prev[i]);
    std_free(dijk -> prev_level[i]);
    std_free(dijk -> dist[i]);
    std_free(dijk -> reached[i]);
    std_free(dijk -> levels[i]);
    std_free(dijk -> metrics[i]);
    std_free(dijk -> width[i]);
  }
  std_free(dijk -> dist);
  std_free(dijk -> prev_level);
  std_free(dijk -> prev);
  std_free(dijk -> levels);
  std_free(dijk -> metrics);
  std_free(dijk -> width);
  std_free(dijk -> reached);
  std_free(dijk);
}

static void
setup(leveled_dijkstra_t dijk, router_graph_link_list_t links, int seed){
  float w;
  router_graph_link_list_cell_t link_cell;
  router_graph_link_t link;

  overlay_node_t n0, n1;
  int pid0, pid1;

  srand(seed);
  
  for(link_cell = router_graph_link_list_head(links);
      link_cell != router_graph_link_list_end(links);
      link_cell = router_graph_link_list_cell_next(link_cell)){

    link = router_graph_link_list_cell_data(link_cell);

    n0 = link -> conn -> n0;
    n1 = link -> conn -> n1;

    //w = 1.0 / link -> conn -> width * 1e6; /* metric for distance */
    w = link -> conn -> len; /* metric for distance */

    assert(link -> conn -> width > 0.0); assert(w > 0.0);
    pid0 = n0 -> pid;
    pid1 = n1 -> pid;

    dijk -> levels[pid0][pid1] = link -> level_0;
    dijk -> levels[pid1][pid0] = link -> level_1;
    dijk -> metrics[pid0][pid1] = w;
    dijk -> metrics[pid1][pid0] = w;
    dijk -> width[pid0][pid1] = link -> conn -> width;
    dijk -> width[pid1][pid0] = link -> conn -> width;
  }

}

/* extract closest un-extracted node */
/* if the distance is the same, choose smaller pid, larger level */
static int
extract_min_node(leveled_dijkstra_t dijk, int *pid, int *level){
  int p, l;
  float val = MAX_DIJKSTRA_DIST;
  *pid = -1; *level = -1;
  for(p = 0; p < dijk -> num_nodes; p++){
    for(l = dijk -> num_levels - 1; l >= 0; l--){
/*     for(l = 0; l < dijk -> num_levels; l++){ */
      if(dijk -> reached[p][l]) /* if node removed, ignore */
	continue;
      if(dijk -> dist[p][l] < val){
	val = dijk -> dist[p][l];
	*pid = p; *level = l;
      }
    }
  }
  
  if(*pid != -1 && *level != -1){
    dijk -> reached[*pid][*level] = 1; /* mark node as removed */
    return 0;
  }else{
    return -1;
  }
}

static void
compute(leveled_dijkstra_t dijk, overlay_node_t src){
  int l;
  int u, v;
  int level;
/*   int total = dijk -> num_nodes * dijk -> num_levels; */
  /* init source node */
  int src_pid = src -> pid;
  for(l = 0; l < dijk -> num_levels; l++){
    dijk -> dist[src_pid][l] = 0.0;
  }

/*   for(src_pid = 0; src_pid < dijk -> num_nodes; src_pid ++){ */
/*     for(l = 0; l < dijk -> num_levels; l++){ */
/*       printf("%.3f ", dijk -> dist[src_pid][l]); */
/*     } */
/*     printf("\n"); */
/*   } */

  /* for(i = 0; i < total; i ++){ */
  while(1){
    /* find next closest (u, l) node */
    if(extract_min_node(dijk, &u, &level) == -1)
      break; /* if no more reachable nodes, exit */

    /* printf("extracted (%d, %d)\n", u, level);fflush(stdout); */

    /* cycle through neighbor edges */
    for(v = 0; v < dijk -> num_nodes; v++){
      /* if lower edge level, ignore */
      if(dijk -> levels[u][v] < level)
	continue;

      /* try out (v, l) such that l >= edge level */
      for(l = dijk -> levels[u][v]; l < dijk -> num_levels; l++){
	if(dijk -> dist[u][level] + dijk -> metrics[u][v] < dijk -> dist[v][l]){
	  dijk -> dist[v][l] = dijk -> dist[u][level] + dijk -> metrics[u][v];

	  assert(dijk -> reached[v][l] == 0);
	  /* set prev pointer */
	  dijk -> prev[v][l] = u;
	  dijk -> prev_level[v][l] = level;

	 /*  printf("(%d,%d) -%.3f(%d)-> (%d, %d): %.3f\n", u, level, dijk -> metrics[u][v], dijk -> levels[u][v], v, l, dijk -> dist[v][l]);fflush(stdout); */
	}
      }
    }
  }
}

/* static void */
/* print_pid_for_vec(long pid){ */
/*   printf("%ld", pid); */
/* } */

static overlay_rtable_t
calc_rt(leveled_dijkstra_t dijk, overlay_node_t src){
  int dst, l, i, hops, metric, *path;
  int node, level, tmp_n, tmp_l;
  float min_dist;
  float min_width;
  long_vector_t node_stack = long_vector_create(1);
  overlay_rtable_t rt;
  overlay_rtable_entry_t rentry;

  int src_pid = src -> pid;
  rt = overlay_rtable_create(src -> pid, dijk -> num_nodes);
  
  for(dst = 0; dst < dijk -> num_nodes; dst ++){
    if(dst == src_pid)
      continue;
    /* for each dst, figure out level that yields shortest path */
    min_dist = MAX_DIJKSTRA_DIST;
    level = -1;
    /* can better diversify path if we try to choose high level paths */
    for(l = dijk -> num_levels - 1; l >= 0; l--){
      if(dijk -> dist[dst][l] < min_dist){
	min_dist = dijk -> dist[dst][l];
	level = l;
      }
    }
    if(level == -1){
      fprintf(stderr, "dstid: %d is unreachable from srcpid: %d\n", dst, src_pid);
      exit(1);
    }

    /* figure out path from src to dst node */
    node = dst;
    min_width = MAX_DIJKSTRA_WIDTH;
    /* printf("level: "); */
    while(1){
      /* printf("%d, ", level); */
      long_vector_add(node_stack, node);
      tmp_l = dijk -> prev_level[node][level];
      tmp_n = dijk -> prev[node][level];

      if(tmp_n == -1)
	break;
      
      min_width = min_width < dijk -> width[tmp_n][node] ? min_width : dijk -> width[tmp_n][node];
      node = tmp_n; level = tmp_l;
    }
    /* long_vector_print(node_stack, print_pid_for_vec); */

    /* store routing information in routing table */
    long_vector_reverse(node_stack);
    hops = long_vector_size(node_stack) - 1; /* because srcnode is included in path */
    path = (int*) std_malloc(sizeof(int) * (hops + 1));
    for(i = 0; i < hops + 1; i++){
      path[i] = (int)long_vector_get(node_stack, i);
    }

    metric = (int)(min_dist);
    rentry = overlay_rtable_entry_create(dst, path, hops, metric, min_width);
    overlay_rtable_add_entry(rt, rentry);

    std_free(path);

    long_vector_clear(node_stack);
  }

  long_vector_destroy(node_stack);

  /* overlay_rtable_print(rt); */

  return rt;
}

overlay_rtable_t
leveled_dijkstra_run(leveled_dijkstra_t dijk, router_graph_link_list_t links, overlay_node_t srcnode, int seed){
  setup(dijk, links, seed);
  compute(dijk, srcnode);
  return calc_rt(dijk, srcnode);
}
