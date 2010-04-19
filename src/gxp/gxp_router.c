#include <xml/topology.h>
#include <xml/parser.h>
#include <struct/rtable.h>
#include <graph/overlay.h>
#include <graph/router.h>
#include <graph/dijkstra.h>
#include "impl/gxp.h"
#include "impl/gxp_router.h"

#include <stdlib.h>

/* TODO: merge gxp_router contents with rtable_planner */
gxp_router_t
gxp_router_create(const char* filename, int seed){
  gxp_router_t router = (gxp_router_t) std_malloc(sizeof(gxp_router));
  xml_topology_t xml_top;
  xml_topology_parser_t parser = xml_topology_parser_create();
  
  xml_top = xml_topology_parser_run(parser, filename);
  /* randomize metric */
  xml_topology_randomize_edge_width(xml_top, seed);
  
  xml_topology_parser_destroy(parser);

  router -> graph = overlay_graph_create();
  router -> xml_top = xml_top;

  return router;
}

void
gxp_router_destroy(gxp_router_t router){
  overlay_graph_destroy(router -> graph);
  xml_topology_destroy(router -> xml_top);
  std_free(router);
}

overlay_rtable_t
gxp_router_run(gxp_router_t router, gxp_man_t gxpman, const int** conn_mat, int rttype, int seed){
  int i, idx;
  float avgdist, *avgdist_map;
  router_graph_link_t link;
  router_graph_link_list_t sptrees;
  leveled_dijkstra_t dijk;
  overlay_edge_t e;
  overlay_rtable_t rt;
  overlay_rtable_entry_t rt_entry;
  int levels, pid, srcid, dstid, dist;
  const char *name;
  overlay_node_t* nodes = (overlay_node_t*)std_malloc(sizeof(overlay_node_t) * gxpman -> gxp_num_execs);

  /* add established connections to overlay map */
  for(pid = 0; pid < gxpman -> gxp_num_execs; pid ++){
    name = gxpman -> peer_hostnames[pid];
    nodes[pid] = overlay_graph_add_node(router -> graph, pid, name);
  }
  for(srcid = 0; srcid < gxpman -> gxp_num_execs; srcid ++){
    for(dstid = 0; dstid < gxpman -> gxp_num_execs; dstid ++){
      if((dist = conn_mat[srcid][dstid]) != GXP_NO_RTT){
	e = overlay_graph_add_conn(router -> graph, router -> xml_top, nodes[srcid], nodes[dstid]);
	e -> len = dist; /* TODO: ad-hoc way to set real rtt as edge length */
      }
    }
  }

  /* make spanning trees */
  switch(rttype){
  case GXP_RT_TYPE_UPDOWN_BFS:
    sptrees = router_graph_make_updown_tree(router -> graph,
					    UPDOWN_ROUTER_BFS, seed);
    dijk = leveled_dijkstra_create(router -> graph -> hosts, 2); /* levels: up-phase, down-phase */
    break;
  case GXP_RT_TYPE_UPDOWN_DFS:
    sptrees = router_graph_make_updown_tree(router -> graph,
					    UPDOWN_ROUTER_DFS, seed);
    dijk = leveled_dijkstra_create(router -> graph -> hosts, 2); /* levels: up-phase, down-phase */
    break;
  case GXP_RT_TYPE_ORDERED_RANDOM:
    sptrees = router_graph_make_spanning_trees(router -> graph, &levels,
					       ORDERED_LINK_ROUTER_RANDOM, seed);
    dijk = leveled_dijkstra_create(router -> graph -> hosts, levels);
    break;
  case GXP_RT_TYPE_ORDERED_BAND:
    sptrees = router_graph_make_spanning_trees(router -> graph, &levels,
					       ORDERED_LINK_ROUTER_BAND, seed);
    dijk = leveled_dijkstra_create(router -> graph -> hosts, levels);
    break;
  case GXP_RT_TYPE_ORDERED_HOPS:
    sptrees = router_graph_make_spanning_trees(router -> graph, &levels,
					       ORDERED_LINK_ROUTER_HOPS, seed);
    dijk = leveled_dijkstra_create(router -> graph -> hosts, levels);
    break;
  case GXP_RT_TYPE_ORDERED_HUB:
    sptrees = router_graph_make_spanning_trees(router -> graph, &levels,
					       ORDERED_LINK_ROUTER_HUB, seed);
    dijk = leveled_dijkstra_create(router -> graph -> hosts, levels);
    break;
  case GXP_RT_TYPE_ORDERED_BFS:
    /* first compute shortest path RT */
    sptrees = router_graph_make_deadlock_prone_links(router -> graph);
    dijk = leveled_dijkstra_create(router -> graph -> hosts, 1);
    rt = leveled_dijkstra_run(dijk, sptrees, nodes[gxpman -> gxp_idx], seed);
    while(router_graph_link_list_size(sptrees)){
      link = router_graph_link_list_popleft(sptrees);
      router_graph_link_destroy(link);
    }
    router_graph_link_list_destroy(sptrees);
    leveled_dijkstra_destroy(dijk);

    /* calculate avg. dist to all nodes from RT */
    avgdist = 0.0;
    for(i = 0; i < overlay_rtable_size(rt); i++){
      if(i == rt -> srcpid) continue;
      rt_entry = overlay_rtable_get_entry(rt, i);
      avgdist += rt_entry -> metric;
    }
    overlay_rtable_destroy(rt);
    
    /* printf("%d: %.3f\n", gxpman -> gxp_idx, avgdist);fflush(stdout); */
    
    fprintf(gxpman -> wfp, "%d %.f\n", gxpman -> gxp_idx, avgdist);
    fflush(gxpman -> wfp);

    /* disseminate to all nodes */
    avgdist_map = std_calloc(gxpman -> gxp_num_execs, sizeof(int));
    for(i = 0; i < gxpman -> gxp_num_execs; i++){
      fscanf(gxpman -> rfp, "%d %f", &idx, &avgdist);
      avgdist_map[idx] = avgdist;
    }

    sptrees = router_graph_make_bfs_spanning_trees(router -> graph, gxpman -> gxp_num_execs,
					       avgdist_map, &levels, seed);
    std_free(avgdist_map);
    dijk = leveled_dijkstra_create(router -> graph -> hosts, levels);
    break;
  case GXP_RT_TYPE_DEADLOCK:
    sptrees = router_graph_make_deadlock_prone_links(router -> graph);
    /* deadlocking, so only 1 level for graph */
    dijk = leveled_dijkstra_create(router -> graph -> hosts, 1);
    break;
  default:
    fprintf(stderr, "unknown GXP_ROUTING_TYPE: %d\n", rttype);
    exit(1);
  }

  /* compute routing table */
  rt = leveled_dijkstra_run(dijk, sptrees, nodes[gxpman -> gxp_idx], seed);

  /* clean up */
  std_free(nodes);
  while(router_graph_link_list_size(sptrees)){
    link = router_graph_link_list_popleft(sptrees);
    router_graph_link_destroy(link);
  }
  router_graph_link_list_destroy(sptrees);
  leveled_dijkstra_destroy(dijk);

  return rt;
}
