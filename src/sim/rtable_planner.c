#include <stdlib.h>

#include <xml/topology.h>
#include <xml/parser.h>
#include <struct/rtable.h>
#include <graph/overlay.h>
#include <graph/router.h>
#include <graph/dijkstra.h>
#include "sim/rtable_planner.h"

rtable_planner_t
rtable_planner_create(const char* filename, int seed){
  rtable_planner_t planner = (rtable_planner_t) std_malloc(sizeof(rtable_planner));
  xml_topology_t xml_top;
  xml_topology_parser_t parser = xml_topology_parser_create();
  
  xml_top = xml_topology_parser_run(parser, filename);
  /* randomize metric */
  xml_topology_randomize_edge_width(xml_top, seed);
  
  xml_topology_parser_destroy(parser);

  planner -> graph = overlay_graph_create();
  planner -> xml_top = xml_top;

  return planner;
}

void
rtable_planner_destroy(rtable_planner_t planner){
  overlay_graph_destroy(planner -> graph);
  xml_topology_destroy(planner -> xml_top);
  std_free(planner);
}

overlay_rtable_t
rtable_planner_run(rtable_planner_t planner, const overlay_node_t srcnode, int rttype, int seed){
  int i, idx, nnodes;
  float avgdist, *avgdist_map;
  router_graph_link_t link;
  router_graph_link_list_t sptrees;
  leveled_dijkstra_t dijk;
  overlay_rtable_t rt;
  overlay_rtable_entry_t rt_entry;
  int levels;

  /* make spanning trees */
  switch(rttype){
  case RT_TYPE_UPDOWN_BFS:
    sptrees = router_graph_make_updown_tree(planner -> graph,
					    UPDOWN_ROUTER_BFS, seed);
    dijk = leveled_dijkstra_create(planner -> graph -> hosts, 2); /* levels: up-phase, down-phase */
    break;
  case RT_TYPE_UPDOWN_DFS:
    sptrees = router_graph_make_updown_tree(planner -> graph,
					    UPDOWN_ROUTER_DFS, seed);
    dijk = leveled_dijkstra_create(planner -> graph -> hosts, 2); /* levels: up-phase, down-phase */
    break;
  case RT_TYPE_ORDERED_RANDOM:
    sptrees = router_graph_make_spanning_trees(planner -> graph, &levels,
					       ORDERED_LINK_ROUTER_RANDOM, seed);
    dijk = leveled_dijkstra_create(planner -> graph -> hosts, levels);
    break;
  case RT_TYPE_ORDERED_BAND:
    sptrees = router_graph_make_spanning_trees(planner -> graph, &levels,
					       ORDERED_LINK_ROUTER_BAND, seed);
    dijk = leveled_dijkstra_create(planner -> graph -> hosts, levels);
    break;
  case RT_TYPE_ORDERED_HOPS:
    sptrees = router_graph_make_spanning_trees(planner -> graph, &levels,
					       ORDERED_LINK_ROUTER_HOPS, seed);
    dijk = leveled_dijkstra_create(planner -> graph -> hosts, levels);
    break;
  case RT_TYPE_ORDERED_HUB:
    sptrees = router_graph_make_spanning_trees(planner -> graph, &levels,
					       ORDERED_LINK_ROUTER_HUB, seed);
    dijk = leveled_dijkstra_create(planner -> graph -> hosts, levels);
    break;
  case RT_TYPE_ORDERED_BFS:
    nnodes = overlay_node_vector_size(planner -> graph -> hosts);
    avgdist_map = std_calloc(nnodes, sizeof(int));

    for(idx = 0; idx < nnodes; idx++){
      /* first compute shortest path RT */
      sptrees = router_graph_make_deadlock_prone_links(planner -> graph);
      dijk = leveled_dijkstra_create(planner -> graph -> hosts, 1);
      rt = leveled_dijkstra_run(dijk, sptrees, overlay_node_vector_get(planner -> graph -> hosts, idx), seed);

      /* clear-up afterwards */
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
    
      avgdist_map[idx] = avgdist;
    }

    sptrees = router_graph_make_bfs_spanning_trees(planner -> graph, nnodes,
					       avgdist_map, &levels, seed);
    std_free(avgdist_map);
    dijk = leveled_dijkstra_create(planner -> graph -> hosts, levels);
    break;
  case RT_TYPE_DEADLOCK:
    sptrees = router_graph_make_deadlock_prone_links(planner -> graph);
    /* deadlocking, so only 1 level for graph */
    dijk = leveled_dijkstra_create(planner -> graph -> hosts, 1);
    break;
  default:
    fprintf(stderr, "unknown ROUTING_TYPE: %d\n", rttype);
    exit(1);
  }

  /* compute routing table */
  rt = leveled_dijkstra_run(dijk, sptrees, srcnode, seed);

  /* clean up */
  while(router_graph_link_list_size(sptrees)){
    link = router_graph_link_list_popleft(sptrees);
    router_graph_link_destroy(link);
  }
  router_graph_link_list_destroy(sptrees);
  leveled_dijkstra_destroy(dijk);

  return rt;
}
