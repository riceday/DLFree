#include <stdlib.h>

#include <std/std.h>
#include <struct/rtable.h>
#include <graph/overlay.h>
#include "sim/rtable_stat.h"
#include "sim/rtable_sim.h"

rtable_simulator_t
rtable_simulator_create(const char* filename, int seed){
  rtable_simulator_t sim = (rtable_simulator_t) std_malloc(sizeof(rtable_simulator));
  sim -> planner = rtable_planner_create(filename, seed);
  return sim;
}

void
rtable_simulator_destroy(rtable_simulator_t sim){
  rtable_planner_destroy(sim -> planner);
  std_free(sim);
}

overlay_rtable_vector_t
rtable_simulator_run(rtable_simulator_t sim, int overlaytype, float param, int rttype, int seed){
  int i, nnodes;
  overlay_node_t srcnode;
  overlay_rtable_t rtable;
  overlay_rtable_vector_t rtable_vec = overlay_rtable_vector_create(1);

  /* create random graph from xml */
  switch(overlaytype){
  case RTABLE_SIM_OVERLAY_TYPE_RANDOM:
    printf("simulator_run: simulating overlay graph: density: %.3f seed: %d\n", param, seed);
    overlay_graph_simulate_random(sim -> planner -> graph, sim -> planner -> xml_top, param, seed);
    break;
  case RTABLE_SIM_OVERLAY_TYPE_LOCALITY:
    printf("simulator_run: simulating overlay graph: alpha: %d seed: %d\n", (int)param, seed);
    overlay_graph_simulate_locality_aware(sim -> planner -> graph, sim -> planner -> xml_top, (int)param, seed);
    break;
  default:
    fprintf(stderr, "rtable_simulator_run: unknown overlay type: %d \n", overlaytype);
    exit(1);
  }

  /* calculate rtable for each srcnode */
  nnodes = overlay_node_vector_size(sim -> planner -> graph -> hosts);
  for(i = 0; i < nnodes; i++){
    srcnode = overlay_node_vector_get(sim -> planner -> graph -> hosts, i);
    /* printf("simulator_run: computing routing table for: %s\n", srcnode -> name); */
    rtable = rtable_planner_run(sim -> planner, srcnode, rttype, seed);
    overlay_rtable_vector_add(rtable_vec, rtable);
  }

  /* calculate statistics on hops */
  overlay_rtable_stats_calc_hops(rtable_vec, nnodes);
  overlay_rtable_stats_print_band_histo(rtable_vec, nnodes, sim -> planner -> xml_top, sim -> planner -> graph, 100);
  overlay_rtable_stats_calc_node_hotspot(rtable_vec, nnodes);
  overlay_rtable_stats_calc_edge_hotspot(rtable_vec, nnodes);
  overlay_rtable_stats_calc_roundabout(rtable_vec, nnodes, sim -> planner -> xml_top, sim -> planner -> graph);

  return rtable_vec;
}

overlay_rtable_vector_t
rtable_simulator_run2(rtable_simulator_t sim, float density, int rttype, int seed){
  int i, j, nnodes;
  overlay_node_t srcnode;
  overlay_rtable_t rtable;
  overlay_rtable_vector_t rtable_vec = overlay_rtable_vector_create(1);
  int **conn_req;
  int conns;

  /* create random graph from xml */
  printf("simulator_run2: simulating overlay graph: density: %.3f seed: %d\n", density, seed);
  overlay_graph_simulate_random(sim -> planner -> graph, sim -> planner -> xml_top, density, seed);
/*   printf("simulator_run2: simulating overlay graph: ring\n"); */
/*   overlay_graph_simulate_ring(sim -> planner -> graph, sim -> planner -> xml_top); */

  while(1){
    /* calculate rtable for each srcnode */
    nnodes = overlay_node_vector_size(sim -> planner -> graph -> hosts);
    for(i = 0; i < nnodes; i++){
      srcnode = overlay_node_vector_get(sim -> planner -> graph -> hosts, i);
      /* printf("simulator_run: computing routing table for: %s\n", srcnode -> name); */
      rtable = rtable_planner_run(sim -> planner, srcnode, rttype, seed);
      overlay_rtable_vector_add(rtable_vec, rtable);
    }

    /* assess bandwidth */
    overlay_rtable_stats_print_band_histo(rtable_vec, nnodes, sim -> planner -> xml_top, sim -> planner -> graph, 10);
    conn_req = overlay_rtable_stats_calc_band(rtable_vec, nnodes, sim -> planner -> xml_top, sim -> planner -> graph);

    /* issue connection updates */
    conns = 0;
    for(i = 0; i < nnodes; i++){
      for(j = i + 1; j < nnodes; j++){
	if(conn_req[i][j]){
	  overlay_graph_add_conn_by_pid(sim -> planner -> graph, sim -> planner -> xml_top, i, j);
	  conns++;
	}
      }
    }

    printf("added %d connections\n", conns);
    if(conns == 0) break;
    
    /* clean up */
    for(i = 0; i < nnodes; i++)
      std_free(conn_req[i]);
    std_free(conn_req);
    while(overlay_rtable_vector_size(rtable_vec)){
      rtable = overlay_rtable_vector_pop(rtable_vec);
      overlay_rtable_destroy(rtable);
    }
  }

  /* calculate statistics on hops */
  overlay_rtable_stats_calc_hops(rtable_vec, nnodes);
  overlay_rtable_stats_print_band_histo(rtable_vec, nnodes, sim -> planner -> xml_top, sim -> planner -> graph, 10);
  overlay_rtable_stats_calc_node_hotspot(rtable_vec, nnodes);
  overlay_rtable_stats_calc_edge_hotspot(rtable_vec, nnodes);
  overlay_rtable_stats_calc_roundabout(rtable_vec, nnodes, sim -> planner -> xml_top, sim -> planner -> graph);

  return rtable_vec;
}
