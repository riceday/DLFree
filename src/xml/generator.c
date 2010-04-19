#include <stdio.h>
#include <stdlib.h>

#include <std/std.h>
#include <std/vector.h>
#include <std/long.h>
#include "xml/parser.h"

enum xml_topology_generator_tags {
  XML_TOPOLOGY_TAG_CLUSTER,
  XML_TOPOLOGY_TAG_SWITCH,
  XML_TOPOLOGY_TAG_NODE,
};

static void
indent_line(FILE *fp, int depth){
  int i;
  for(i = 0; i < depth; i++)
    fprintf(fp, " ");
}

static void
push_element(FILE *fp, int type, long_vector_t elemstack, float band, const char *hostname){
  indent_line(fp, long_vector_size(elemstack));
  switch(type){
  case XML_TOPOLOGY_TAG_CLUSTER:
    fprintf(fp, "<%s>\n", XML_CLUSTER_ELEMENT);
    long_vector_add(elemstack, type);
    break;
  case XML_TOPOLOGY_TAG_SWITCH:
    fprintf(fp, "<%s bandwidth=\"%.1f\">\n", XML_SWITCH_ELEMENT, band);
    long_vector_add(elemstack, type);
    break;
  case XML_TOPOLOGY_TAG_NODE:
    fprintf(fp, "<%s bandwidth=\"%.1f\" hostname=\"%s\" />\n", XML_NODE_ELEMENT, band, hostname);
    break;
  }
}

static void
pop_element(FILE *fp, long_vector_t elemstack){
  int type = long_vector_pop(elemstack);
  indent_line(fp, long_vector_size(elemstack));
  switch(type){
  case XML_TOPOLOGY_TAG_CLUSTER:
    fprintf(fp, "</%s>\n", XML_CLUSTER_ELEMENT);
    break;
  case XML_TOPOLOGY_TAG_SWITCH:
    fprintf(fp, "</%s>\n", XML_SWITCH_ELEMENT);
    break;
  }
}

void
xml_topology_generate(const char *filename,
		      int nnodes,
		      float min_band,
		      float max_band,
		      int seed
		      ){
  float band;
  char hostname[20];
  int c = 0;
  int event;
  int node_added = 0;
  long_vector_t elemstack = long_vector_create(1);
  FILE *fp = std_fopen(filename, "w");

  srand(seed);

  push_element(fp, XML_TOPOLOGY_TAG_CLUSTER, elemstack, -2.0, NULL);
  push_element(fp, XML_TOPOLOGY_TAG_SWITCH, elemstack, -2.0, NULL);

  while(long_vector_size(elemstack)){
    event = rand() % 3; /* kinda ad-hoc */
    switch(event){
    case 0: /* new host */
      if(c == nnodes) continue;
      sprintf(hostname, "NODE:%d", c ++);
      band = (float)rand() / RAND_MAX * (max_band - min_band) + min_band;
      push_element(fp, XML_TOPOLOGY_TAG_NODE, elemstack, band, hostname);
      node_added = 1;
      break;
    case 1: /* new switch */
      if(c == nnodes) continue;
      band = (float)rand() / RAND_MAX * (max_band - min_band) + min_band;
      push_element(fp, XML_TOPOLOGY_TAG_SWITCH, elemstack, band, "SWITCH");
      node_added = 0;
      break;
    case 2: /* close switch */
      if(node_added == 0) continue;
      if(long_vector_size(elemstack) <= 2 && c < nnodes) continue;
      pop_element(fp, elemstack);
      break;
    }
  }

  long_vector_destroy(elemstack);
}

static void
myabort(char **argv, const char *msg){
  fprintf(stderr, "%s\n", msg);
  fprintf(stderr, "usage: %s <filname> <nnodes> <seed>\n", argv[0]);
  exit(1);
}

static void
parse_args(int argc, char **argv, char *filename, int *nnodes, int *seed){
  if(argc < 4)
    myabort(argv, "too few argument");
  if(sscanf(argv[1], "%s", filename) != 1)
    myabort(argv, "invalid xml-filename");
  if(sscanf(argv[2], "%d", nnodes) != 1)
    myabort(argv, "invalid density value");
  if(sscanf(argv[3], "%d", seed) != 1)
    myabort(argv, "invalid seed value");
}

int main(int argc, char **argv){
  char filename[1024];
  int nnodes, seed;
  parse_args(argc, argv, filename, &nnodes, &seed);
  xml_topology_generate(filename, nnodes, 100, 1000, seed);
  return 0;
}
