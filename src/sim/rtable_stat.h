#ifndef __RTABLE_STAT_H__
#define __RTABLE_STAT_H__

#include <struct/rtable.h>
#include <xml/topology.h>
#include <graph/overlay.h>

void overlay_rtable_stats_calc_hops(overlay_rtable_vector_t rt_vec, int nnodes);
void overlay_rtable_stats_print_band_histo(overlay_rtable_vector_t rt_vec, int nnodes, xml_topology_t xml_top, overlay_graph_t graph, int nbins);
int** overlay_rtable_stats_calc_band(overlay_rtable_vector_t rt_vec, int nnodes, xml_topology_t xml_top, overlay_graph_t graph);
void overlay_rtable_stats_calc_node_hotspot(overlay_rtable_vector_t rt_vec, int nnodes);
void overlay_rtable_stats_calc_edge_hotspot(overlay_rtable_vector_t rt_vec, int nnodes);
void overlay_rtable_stats_calc_roundabout(overlay_rtable_vector_t rt_vec, int nnodes, xml_topology_t xml_top, overlay_graph_t graph);

#endif // __RTABLE_STAT_H__
