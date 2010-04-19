#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <struct/rtable.h>
#include "sim/rtable_sim.h"

static void
myabort(char **argv, const char *msg){
  fprintf(stderr, "%s\n", msg);
  fprintf(stderr, "usage: %s <xml file> <density> <seed> <rttype>\n", argv[0]);
  exit(1);
}

static void
parse_args(int argc, char **argv, char *filename, float *density, int *seed, int *rttype){
  if(argc < 5)
    myabort(argv, "too few argument");
  if(sscanf(argv[1], "%s", filename) != 1)
    myabort(argv, "invalid xml-filename");
  if(sscanf(argv[2], "%f", density) != 1)
    myabort(argv, "invalid density value");
  if(sscanf(argv[3], "%d", seed) != 1)
    myabort(argv, "invalid seed value");

  if(strcmp(argv[4], "updown") == 0) *rttype = RT_TYPE_UPDOWN_BFS;
  else if(strcmp(argv[4], "updown-dfs") == 0) *rttype = RT_TYPE_UPDOWN_DFS;
  else if(strcmp(argv[4], "random") == 0) *rttype = RT_TYPE_ORDERED_RANDOM;
  else if(strcmp(argv[4], "band") == 0) *rttype = RT_TYPE_ORDERED_BAND;
  else if(strcmp(argv[4], "hops") == 0) *rttype = RT_TYPE_ORDERED_HOPS;
  else if(strcmp(argv[4], "hub") == 0) *rttype = RT_TYPE_ORDERED_HUB;
  else if(strcmp(argv[4], "bfs") == 0) *rttype = RT_TYPE_ORDERED_BFS;
  else if(strcmp(argv[4], "deadlock") == 0) *rttype = RT_TYPE_DEADLOCK;
  else myabort(argv, "invalid rt-type string: allowed ['updown', 'updown-dfs', 'random', 'band', 'hops', 'hub', 'bfs', 'deadlock']");
  
}

int main(int argc, char **argv){
  char xmlfilename[1024];
  float param;
  int seed, rttype = 0;
  rtable_simulator_t sim;
  overlay_rtable_vector_t rt_vec;
  
  parse_args(argc, argv, xmlfilename, &param, &seed, &rttype);

  sim = rtable_simulator_create(xmlfilename, seed);
/*   rt_vec = rtable_simulator_run(sim, RTABLE_SIM_OVERLAY_TYPE_RANDOM, param, rttype, seed); */
  rt_vec = rtable_simulator_run(sim, RTABLE_SIM_OVERLAY_TYPE_LOCALITY, param, rttype, seed);
  
/*   rt_vec = rtable_simulator_run2(sim, density, rttype, seed); */

  /* clean up */
  while(overlay_rtable_vector_size(rt_vec))
    overlay_rtable_destroy(overlay_rtable_vector_pop(rt_vec));
  overlay_rtable_vector_destroy(rt_vec);
  rtable_simulator_destroy(sim);

  return 0;
}
