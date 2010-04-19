#include <dlfree/dlfree.h>
#include <dlfree/gxp.h>
#include <stdio.h>
#include <stdlib.h>
#include "collective.h"

static void
myabort(char** argv, const char *msg){
  fprintf(stderr, "%s\n", msg);
  fprintf(stderr, "usage: %s <xml> <msg size[MB]> [<alpha:1> <iter:10> [<seed:1>]]\n", argv[0]);
  exit(1);
}

static void
parseopt(int argc, char** argv, char *filename, float *size, int *alpha, int *iter, int *seed){
  *alpha = 1;
  *iter = 10;
  *seed = 1;
  
  if(argc < 3)
    myabort(argv, "too few arguments");
  if(sscanf(argv[1], "%s", filename) != 1)
    myabort(argv, "invalid xml-filename");
  if(sscanf(argv[2], "%f", size) != 1)
    myabort(argv, "invalid message size value");
  
  if(argc > 3){
    if(sscanf(argv[3], "%d", alpha) != 1)
      myabort(argv, "invalid alpha value");
  }
  
  if(argc > 4){
    if(sscanf(argv[4], "%d", iter) != 1)
      myabort(argv, "invalid iteration value");
  }
  
  if(argc > 5){
    if(sscanf(argv[5], "%d", seed) != 1)
      myabort(argv, "invalid seed value");
  }
}

int
main(int argc, char** argv){
  gxp_man_t gxp;
  dlfree_comm_node_t comm_node;
  char filename[100];
  float size;
  int alpha, iter, seed;

  parseopt(argc, argv, filename, &size, &alpha, &iter, &seed);

  /* initialization */
  gxp = gxp_man_create();
  comm_node = dlfree_comm_node_create(gxp_man_peer_id(gxp));
  gxp_man_init(gxp);

  /* connect in locality-aware manner */
  gxp_man_connect_locality_aware(gxp, comm_node, filename, alpha, seed);

  /* compute dlfree routing table */
  
  gxp_man_compute_rt(gxp, comm_node, filename, GXP_RT_TYPE_UPDOWN_DFS, seed);
  
  /* do some communication */
  send_all2all(gxp, comm_node, 1024 * 1024 * size, iter);


  /* finalization */
  gxp_man_sync(gxp);
  dlfree_comm_node_destroy(comm_node);
  gxp_man_destroy(gxp);
  return 0;
}
