#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <std/std.h>
#include "graph/overlay.h"
#include "graph/router.h"

LIST_MAKE_TYPE_IMPLEMENTATION(router_graph_link)
VECTOR_MAKE_TYPE_IMPLEMENTATION(router_graph_link)
UNIONSET_MAKE_TYPE_IMPLEMENTATION(overlay_node)

LIST_MAKE_TYPE_IMPLEMENTATION(router_bfs_node)
VECTOR_MAKE_TYPE_IMPLEMENTATION(router_bfs_node)

router_graph_link_t
router_graph_link_create(overlay_edge_t conn){
  router_graph_link_t link = std_malloc(sizeof(router_graph_link));
  link -> conn = conn;

  link -> up = NULL;
  link -> down = NULL;
  link -> level_0 = -1;
  link -> level_1 = -1;

  return link;
}

void
router_graph_link_destroy(router_graph_link_t link){
  /* we do not free link -> conn because it is a weak ref */
  std_free(link);
}

static void
choose_sp_tree_links(overlay_node_vector_t hosts,
		     router_graph_link_list_t pool,
		     router_graph_link_list_t remain,
		     router_graph_link_list_t assigned,
		     int level){
  int i;
  router_graph_link_t link;
  overlay_node_union_set_t link_uset;
  overlay_node_t node, n0, n1;
  
  /* set up union set */
  link_uset = overlay_node_union_set_create();
  for(i = 0; i < overlay_node_vector_size(hosts); i++){
    node = overlay_node_vector_get(hosts, i);
    overlay_node_union_set_add(link_uset, node);
  }
  
  /* iterate while there are still links */
  while(router_graph_link_list_size(pool)){
    link = router_graph_link_list_popleft(pool);
    n0 = link -> conn -> n0;
    n1 = link -> conn -> n1;
    
    if(overlay_node_union_set_is_same_set(link_uset, n0, n1)){ /* if link creates cycle, retain it */
      router_graph_link_list_append(remain, link);
    }else{
      overlay_node_union_set_merge_set(link_uset, n0, n1); /* if suitable link, label it with level */
      link -> level_0 = level;
      link -> level_1 = level;
      router_graph_link_list_append(assigned, link);
    }
  }
  overlay_node_union_set_destroy(link_uset);
}

router_graph_link_list_t
router_graph_make_spanning_trees(overlay_graph_t graph, int *level, int type, int seed){
  int i, e_count;
  router_graph_link_t link;
  router_graph_link_list_t pool, remain, assigned, tmp;
  overlay_edge_vector_t conns = graph -> conns;
  overlay_edge_t conn;

  /* first sort nodes */
  switch(type){
  case ORDERED_LINK_ROUTER_RANDOM:
    /* printf("ORDERED-LINK: RANDOM:\n");fflush(stdout); */
    /* randomly shuffle links */
    overlay_edge_vector_shuffle(conns, seed);
    break;
  case ORDERED_LINK_ROUTER_BAND:
    /* printf("ORDERED-LINK: BAND:\n");fflush(stdout); */
    /* sort by larger bandwidth link */
    overlay_edge_vector_sort(conns, overlay_edge_sort_by_width);
    break;
  case ORDERED_LINK_ROUTER_HOPS:
    /* printf("ORDERED-LINK: HOPS:\n");fflush(stdout); */
    /* sort by links with fewer hops */
    overlay_edge_vector_sort(conns, overlay_edge_sort_by_length);
    break;
  case ORDERED_LINK_ROUTER_HUB:
    /* printf("ORDERED-LINK: HUB:\n");fflush(stdout); */
    /* sort by links connected to hubs */
    overlay_edge_vector_sort(conns, overlay_edge_sort_by_degree);
    break;
  default:
    fprintf(stderr, "unknown ordered-link router type: %d\n", type);
    exit(1);
  }

  pool = router_graph_link_list_create();  
  remain = router_graph_link_list_create();  
  assigned = router_graph_link_list_create();  

  /* populate link pool */
  for(i = 0; i < overlay_edge_vector_size(conns); i++){
    conn = overlay_edge_vector_get(conns, i);
    link = router_graph_link_create(conn);
    router_graph_link_list_append(pool, link);
  }

  /* extract spanning trees until no links left */
  for(*level = 0; router_graph_link_list_size(pool) > 0; (*level) ++){
    e_count = router_graph_link_list_size(pool);
    choose_sp_tree_links(graph -> hosts, pool, remain, assigned, *level);

    assert(router_graph_link_list_size(pool) == 0);
  
    /* use remaining edges to make next spanning tree */
    tmp = pool;
    pool = remain;
    remain = tmp;

/*     printf("made spanning tree level: %d edges: %d\n", *level, e_count - router_graph_link_list_size(pool)); */
  }

  router_graph_link_list_destroy(pool);
  router_graph_link_list_destroy(remain);

  return assigned;
}

static overlay_node_t
updown_choose_root_node(overlay_node_vector_t hosts){
  int nnodes = overlay_node_vector_size(hosts);
  int root_idx = rand() % nnodes;
  return overlay_node_vector_get(hosts, root_idx);
}

static router_graph_link_list_t
updown_assign_ids(overlay_node_t root, int nnodes, overlay_edge_vector_t conns){
  router_graph_link_list_t links;
  router_graph_link_t link;
  overlay_node_t node, child;
  overlay_edge_t conn;
  overlay_node_list_t node_queue = overlay_node_list_create();
  int *assignids = (int*) std_calloc(nnodes, sizeof(int));
  int *depths = (int*) std_calloc(nnodes, sizeof(int));
  int i;
  int idc = 0;

  /* init and setup root */
  memset(assignids, -1, sizeof(int) * nnodes);
  depths[root -> pid] = 0;
  assignids[root -> pid] = idc ++;

  /* assign ids and depth via breadth-first search */
  overlay_node_list_append(node_queue, root);
  while(overlay_node_list_size(node_queue)){
    node = overlay_node_list_popleft(node_queue);

    for(i = 0; i < overlay_edge_vector_size(node -> conns); i++){
      conn = overlay_edge_vector_get(node -> conns, i);
      child = conn -> n0 == node ? conn -> n1 : conn -> n0;

      /* if already visited, ignore */
      if(assignids[child -> pid] != -1)
	continue;

      depths[child -> pid] = depths[node -> pid] + 1;
      assignids[child -> pid] = idc ++;

      overlay_node_list_append(node_queue, child);
    }
  }

  for(i = 0; i < nnodes; i++)
    assert(assignids[i] != -1);

  /* assign up/down to each link */
  links = router_graph_link_list_create();
  for(i = 0; i < overlay_edge_vector_size(conns); i++){
    conn = overlay_edge_vector_get(conns, i);
    link = router_graph_link_create(conn);

    if(depths[conn -> n0 -> pid] < depths[conn -> n1 -> pid]){
      link -> up = conn -> n0;
      link -> down = conn -> n1;
      link -> level_0 = 1;
      link -> level_1 = 0;
    }
    else if(depths[conn -> n0 -> pid] > depths[conn -> n1 -> pid]){
      link -> up = conn -> n1;
      link -> down = conn -> n0;
      link -> level_0 = 0;
      link -> level_1 = 1;
    }
    else{
      if(assignids[conn -> n0 -> pid] < assignids[conn -> n1 -> pid]){
	link -> up =  conn -> n0;
	link -> down =  conn -> n1;
	link -> level_0 = 1;
	link -> level_1 = 0;
      }else{
	link -> up = conn -> n1;
	link -> down = conn -> n0;
	link -> level_0 = 0;
	link -> level_1 = 1;
      }
    }
    /* printf("edge %s -> %s end # UP\n", link -> down -> name, link -> up -> name); */
    router_graph_link_list_append(links, link);
  }
  
  overlay_node_list_destroy(node_queue);
  std_free(assignids);
  std_free(depths);

  return links;
}

static void
dfs_traverse(overlay_node_t node, int *visited, int *idc){
  int i;
  overlay_edge_vector_t conns = overlay_edge_vector_copy(node -> conns);
  overlay_edge_t conn;
  overlay_node_t peer;

  visited[node -> pid] = (*idc) ++;

  /* printf("dfs_traverse: %d %s\n", visited[node -> pid], node -> name); */

  /* first assign priority to edges based on proximity */
  overlay_edge_vector_sort(conns, overlay_edge_sort_by_length);

  for(i = 0; i < overlay_edge_vector_size(conns); i++){
    conn = overlay_edge_vector_get(conns, i);
    peer = conn -> n0 -> pid == node -> pid ? conn -> n1 : conn -> n0;
    if(visited[peer -> pid] != -1)
      continue;
    dfs_traverse(peer, visited, idc);
  }

  overlay_edge_vector_destroy(conns);
}

static router_graph_link_list_t
updown_dfs_assign_ids(overlay_node_t root, int nnodes, overlay_edge_vector_t conns){
  int i;
  overlay_edge_t conn;
  router_graph_link_list_t links;
  router_graph_link_t link;
  int *visited = (int*) std_calloc(nnodes, sizeof(int));
  int idc = 0;
  memset(visited, -1, sizeof(int) * nnodes);

  dfs_traverse(root, visited, &idc);
  
  /* assign up/down to each link */
  links = router_graph_link_list_create();
  for(i = 0; i < overlay_edge_vector_size(conns); i++){
    conn = overlay_edge_vector_get(conns, i);
    link = router_graph_link_create(conn);

    if(visited[conn -> n0 -> pid] < visited[conn -> n1 -> pid]){
      link -> up = conn -> n0;
      link -> down = conn -> n1;
      link -> level_0 = 1;
      link -> level_1 = 0;
    }
    else if(visited[conn -> n0 -> pid] > visited[conn -> n1 -> pid]){
      link -> up = conn -> n1;
      link -> down = conn -> n0;
      link -> level_0 = 0;
      link -> level_1 = 1;
    }
    /* printf("edge %s -> %s end # UP\n", link -> down -> name, link -> up -> name); */
    router_graph_link_list_append(links, link);
  }
  
  /* clean up */
  std_free(visited);
  return links;
}

router_graph_link_list_t
router_graph_make_updown_tree(overlay_graph_t graph, int type, int seed){
  overlay_node_t root;

  srand(seed);
  root = updown_choose_root_node(graph -> hosts);

  switch(type){
  case UPDOWN_ROUTER_BFS:
    /* printf("UPDOWN-BFS:\n");fflush(stdout); */
    return updown_assign_ids(root,
			     overlay_node_vector_size(graph -> hosts),
			     graph -> conns);
  case UPDOWN_ROUTER_DFS:
    /* printf("UPDOWN-DFS:\n");fflush(stdout); */
    return updown_dfs_assign_ids(root,
				 overlay_node_vector_size(graph -> hosts),
				 graph -> conns);
  default:
    fprintf(stderr, "unknown updown router type: %d\n", type);
    exit(1);
  }
}

router_bfs_node_t
router_bfs_node_create(int pid, float val){
  router_bfs_node_t bfs_node = std_malloc(sizeof(router_bfs_node));
  bfs_node -> pid = pid;
  bfs_node -> value = val;
  bfs_node -> links = router_graph_link_vector_create(1);
  return bfs_node;
}

void
router_bfs_node_destroy(router_bfs_node_t bfs_node){
  router_graph_link_vector_destroy(bfs_node -> links);
  std_free(bfs_node);
}

void
router_bfs_node_print(router_bfs_node_t bfs_node){
  printf("node[%d]: %.3f", bfs_node -> pid, bfs_node -> value);
}

static int
router_bfs_node_sorter(router_bfs_node_t n0, router_bfs_node_t n1){
  if(n0 -> value < n1 -> value) return 1;
  else if(n0 -> value == n1 -> value) return 0;
  else return -1;
}

static int
router_graph_do_bfs_search(router_bfs_node_t bfs_node, int nnodes, router_bfs_node_t *bfs_node_map, int level){
  int link_labeled = 0;
  int i, pid;
  router_graph_link_t link;
  router_bfs_node_t bfs_node_next;
  router_bfs_node_list_t node_queue;
  int *visited = std_calloc(nnodes, sizeof(int));

  /* push root node */
  node_queue = router_bfs_node_list_create();
  router_bfs_node_list_append(node_queue, bfs_node);
  visited[bfs_node -> pid] = 1;
  
  while(router_bfs_node_list_size(node_queue)){
    bfs_node = router_bfs_node_list_popleft(node_queue);
    for(i = 0; i < router_graph_link_vector_size(bfs_node -> links); i++){
      link = router_graph_link_vector_get(bfs_node -> links, i);
      
      /* if link is already used and labeled, skip */
      if(link -> level_0 != -1 || link -> level_1 != -1)
	continue;

      /* get node at other end of link */
      pid = link -> conn -> n0 -> pid == bfs_node -> pid ?
	link -> conn -> n1 -> pid : link -> conn -> n0 -> pid;
      
      if(visited[pid] == 0){
	bfs_node_next = bfs_node_map[pid];
	link -> level_0 = level;
	link -> level_1 = level;
	link_labeled = 1;
	router_bfs_node_list_append(node_queue, bfs_node_next);
	visited[pid] = 1;
	/* printf("edge %d -> %d color=%d end\n", bfs_node -> pid, pid, level);fflush(stdout); */
      }
    }
  }
  
  router_bfs_node_list_destroy(node_queue);
  std_free(visited);
  
  return link_labeled;
}

router_graph_link_list_t
router_graph_make_bfs_spanning_trees(overlay_graph_t graph, int nnodes, float* avgdist_map, int *level, int seed){
  int i, pid0, pid1;
  overlay_edge_vector_t conns = graph -> conns;
  overlay_edge_t conn;
  router_graph_link_t link;

  router_bfs_node_t bfs_node;

  router_bfs_node_t *bfs_node_map = (router_bfs_node_t*)std_calloc(nnodes, sizeof(router_bfs_node_t));
  router_bfs_node_vector_t bfs_node_vec = router_bfs_node_vector_create(1);
  router_graph_link_list_t pool = router_graph_link_list_create();
  
/*   iterate over all connections, and construct */
/*   data structures necessary for bfs node search */
  for(i = 0; i < overlay_edge_vector_size(conns); i++){
    conn = overlay_edge_vector_get(conns, i);
    link = router_graph_link_create(conn);
    router_graph_link_list_append(pool, link);

    pid0 = conn -> n0 -> pid;
    pid1 = conn -> n1 -> pid;
    if((bfs_node = bfs_node_map[pid0]) == NULL){
      bfs_node = router_bfs_node_create(pid0, avgdist_map[pid0]);
      bfs_node_map[pid0] = bfs_node;
      router_bfs_node_vector_add(bfs_node_vec, bfs_node);
    }
    router_graph_link_vector_add(bfs_node -> links, link);

    if((bfs_node = bfs_node_map[pid1]) == NULL){
      bfs_node = router_bfs_node_create(pid1, avgdist_map[pid1]);
      bfs_node_map[pid1] = bfs_node;
      router_bfs_node_vector_add(bfs_node_vec, bfs_node);
    }
    router_graph_link_vector_add(bfs_node -> links, link);
  }
  for(i = 0; i < router_bfs_node_vector_size(bfs_node_vec); i++){
    bfs_node = router_bfs_node_vector_get(bfs_node_vec, i);
    router_graph_link_vector_shuffle(bfs_node -> links, seed);
  }

  /* sort bfs nodes according to avg distance */
  router_bfs_node_vector_sort(bfs_node_vec, router_bfs_node_sorter);
  /* router_bfs_node_vector_print(bfs_node_vec, router_bfs_node_print);fflush(stdout); */

  /* do bfs starting from each node, in order of sort */
  for(i = 0, *level = 0; i < router_bfs_node_vector_size(bfs_node_vec); i++){
    bfs_node = router_bfs_node_vector_get(bfs_node_vec, i);

    /* if at least 1 link was labeled, increment level */
    if(router_graph_do_bfs_search(bfs_node, nnodes, bfs_node_map, *level))
      (*level) ++;
  }

  /* clean up */
  std_free(bfs_node_map);
  while(router_bfs_node_vector_size(bfs_node_vec)){
    bfs_node = router_bfs_node_vector_pop(bfs_node_vec);
    router_bfs_node_destroy(bfs_node);
  }
  router_bfs_node_vector_destroy(bfs_node_vec);
  
  return pool;
}

router_graph_link_list_t
router_graph_make_deadlock_prone_links(overlay_graph_t graph){
  int i;
  router_graph_link_t link;
  router_graph_link_list_t pool;
  overlay_edge_vector_t conns = graph -> conns;
  overlay_edge_t conn;

  pool = router_graph_link_list_create();  

  /* ASSIGN LEVEL 0 TO ALL LINKS : THIS WILL YIELD DEADLOCKING ROUTING-TABLE */
  for(i = 0; i < overlay_edge_vector_size(conns); i++){
    conn = overlay_edge_vector_get(conns, i);
    link = router_graph_link_create(conn);
    link -> level_0 = 0;
    link -> level_1 = 0;
    router_graph_link_list_append(pool, link);
  }

  return pool;
}
