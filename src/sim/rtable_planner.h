#ifndef __RTABLE_PLANNER_H__
#define __RTABLE_PLANNER_H__
#include <xml/topology.h>
#include <graph/overlay.h>
#include <struct/rtable.h>

enum rtable_planner_rt_types {
  RT_TYPE_UPDOWN_BFS,
  RT_TYPE_UPDOWN_DFS,
  RT_TYPE_ORDERED_RANDOM,
  RT_TYPE_ORDERED_BAND,
  RT_TYPE_ORDERED_HOPS,
  RT_TYPE_ORDERED_HUB,
  RT_TYPE_ORDERED_BFS,
  RT_TYPE_DEADLOCK,
};

typedef struct rtable_planner{
  xml_topology_t xml_top;
  overlay_graph_t graph;
} rtable_planner, *rtable_planner_t;

rtable_planner_t rtable_planner_create(const char* filename, int seed);
void rtable_planner_destroy(rtable_planner_t planner);
overlay_rtable_t rtable_planner_run(rtable_planner_t planner, const overlay_node_t srcnode, int rttype, int seed);

#endif // __RTABLE_PLANNER_H__
