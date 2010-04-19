#include <dlfree/dlfree.h>
#include <dlfree/gxp.h>
#include "collective.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void
myabort(char** argv, const char *msg){
  fprintf(stderr, "%s\n", msg);
  fprintf(stderr, "usage: %s <xml> <density/alpha/ngates> <size[MB]> <rt-type> [<iter:10> [<seed:1>]]\n", argv[0]);
  exit(1);
}

static void
parseopt(int argc, char** argv, char *filename, float *overlay_param, float *size, int *rttype, int *iter, int *seed){
  if(argc < 5)
    myabort(argv, "too few arguments");
  if(sscanf(argv[1], "%s", filename) != 1)
    myabort(argv, "invalid xml-filename");
  if(sscanf(argv[2], "%f", overlay_param) != 1)
    myabort(argv, "invalid overlay parameter");
  if(sscanf(argv[3], "%f", size) != 1)
    myabort(argv, "invalid size value");
  if(strcmp(argv[4], "updown") == 0)
    *rttype = GXP_RT_TYPE_UPDOWN_BFS;
  else if(strcmp(argv[4], "updown-dfs") == 0)
    *rttype = GXP_RT_TYPE_UPDOWN_DFS;
  else if(strcmp(argv[4], "random") == 0)
    *rttype = GXP_RT_TYPE_ORDERED_RANDOM;
  else if(strcmp(argv[4], "band") == 0)
    *rttype = GXP_RT_TYPE_ORDERED_BAND;
  else if(strcmp(argv[4], "hops") == 0)
    *rttype = GXP_RT_TYPE_ORDERED_HOPS;
  else if(strcmp(argv[4], "hub") == 0)
    *rttype = GXP_RT_TYPE_ORDERED_HUB;
  else if(strcmp(argv[4], "bfs") == 0)
    *rttype = GXP_RT_TYPE_ORDERED_BFS;
  else if(strcmp(argv[4], "deadlock") == 0)
    *rttype = GXP_RT_TYPE_DEADLOCK;
  else{
    *rttype = -1;
    myabort(argv, "invalid rt-type: 'updown', 'updown-dfs', 'random', 'band', 'hops', 'hub', 'bfs', 'deadlock'");
  }

  if(argc > 5){
    if(sscanf(argv[5], "%d", iter) != 1)
      myabort(argv, "invalid iteration value");
  }else{
    *iter = 10;
  }
  if(argc > 6){
    if(sscanf(argv[6], "%d", seed) != 1)
      myabort(argv, "invalid seed value");
  }else{
    *seed = 1;
  }
}

int
main(int argc, char** argv){
  gxp_man_t gxp;
  dlfree_comm_node_t comm_node;
  char filename[100];

  int iter, rttype, seed;
  float topology_param, size;

  parseopt(argc, argv, filename, &topology_param, &size, &rttype, &iter, &seed);

  gxp = gxp_man_create();
  comm_node = dlfree_comm_node_create(gxp_man_peer_id(gxp));
  gxp_man_init(gxp);

  if (1) {
    /* if necessary, set address priority: */
    /* below we give priority to 10.**, then 192.**, 
       then any iface (excluding 10.**, 127.**) */
    const char *prio[] = {"10.5"};
    gxp_man_set_iface_prio(gxp, prio, sizeof(prio) / sizeof(prio[0]));
  }

  /* first choose connection scheme */

#if 0
   gxp_man_connect_all(gxp, comm_node); 
#elif 0
   gxp_man_connect_random(gxp, comm_node, topology_param, seed); 
#elif 1
   gxp_man_connect_ring(gxp, comm_node);
#elif 0
   gxp_man_connect_line(gxp, comm_node);
#elif 0
   gxp_man_connect_with_gateway(gxp, comm_node, 5, (int)topology_param, density, seed);
#elif 0
  gxp_man_connect_locality_aware(gxp, comm_node, filename, (int)topology_param, seed);
#else
#error specify connection scheme
#endif

  /* compute dlfree routing table */
  
  gxp_man_compute_rt(gxp, comm_node, filename, rttype, seed);
  
  gxp_man_sync(gxp);

  /* do some communication */

#if 1
  send_all2all(gxp, comm_node, 1024 * 1024 * size, iter);
#elif 0
  send_all2one(gxp, comm_node, /* dst_idx = */ 0, 1024 * 1024 * size, iter);
#elif 0
  send_one2all(gxp, comm_node, /* src_idx = */ 0, 1024 * 1024 * size, iter);
#else
#error choose a communication pattern
#endif

  gxp_man_sync(gxp);
  
  dlfree_comm_node_destroy(comm_node);
  
  gxp_man_destroy(gxp);
  return 0;
}
