#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <expat.h>

#include <std/std.h>
#include "xml/parser.h"

xml_topology_parser_t
xml_topology_parser_create(){
  xml_topology_parser_t parser = (xml_topology_parser_t)std_malloc(sizeof(xml_topology_parser));

  parser -> switch_id = 0;
  parser -> node_stack = xml_top_node_vector_create(1);
  parser -> topology = NULL;
  return parser;
}

void
xml_topology_parser_destroy(xml_topology_parser_t parser){
  xml_top_node_vector_destroy(parser -> node_stack);
  assert(parser -> topology == NULL);
  std_free(parser);
}

void
xml_topology_parser_startElem(void *userdata, const XML_Char *name, const XML_Char **attrs){
  xml_topology_parser_t parser = (xml_topology_parser_t)userdata;
  xml_top_node_t node, parent;

  int i;
  
  float width = -1.0f;
  float len = 1.0f;
  
  char nodename[20];
  if(strcmp(XML_CLUSTER_ELEMENT, name) == 0){
    node = xml_topology_add_node(parser -> topology, XML_ROOT_NAME_PREFIX, NULL); /* create root node */
  }
  else if(strcmp(XML_SWITCH_ELEMENT, name) == 0){
    sprintf(nodename, "%s:%d", XML_SWITCH_NAME_PREFIX, parser -> switch_id ++);

    for(i = 0; attrs[i] != NULL; i+=2){
      if(strcmp(attrs[i], "bandwidth") == 0){
	sscanf(attrs[i+1], "%f", &width);
      }
    }
    
    parent = xml_top_node_vector_last(parser -> node_stack);
    node = xml_topology_add_edge(parser -> topology, parent, nodename, len, width);
  }
  else if(strcmp(XML_NODE_ELEMENT, name) == 0){
    
    for(i = 0; attrs[i] != NULL; i+=2){
      if(strcmp(attrs[i], "bandwidth") == 0){
	sscanf(attrs[i+1], "%f", &width);
      }
      else if(strcmp(attrs[i], "hostname") == 0){
	sscanf(attrs[i+1], "%s", nodename);
      }
    }
    
    parent = xml_top_node_vector_last(parser -> node_stack);
    node = xml_topology_add_edge(parser -> topology, parent, nodename, len, width);
  }else{
    fprintf(stderr, "Unknown Element Type: %s\n", name);
    exit(1);
  }

  xml_top_node_vector_add(parser -> node_stack, node);
  
}

void
xml_topology_parser_endElem(void *userdata, const XML_Char *name){
  xml_topology_parser_t parser = (xml_topology_parser_t)userdata;

  xml_top_node_vector_pop(parser -> node_stack);
}

xml_topology_t
xml_topology_parser_run(xml_topology_parser_t parser, const char* filename){
  char buff[XML_PARSER_BUFF_SIZE];
  FILE *fp = std_fopen(filename, "r");
  int done = 0;
  int len;
  int stat;
  xml_topology_t topology = xml_topology_create();
  
  /* setup parser */
  XML_Parser p = XML_ParserCreate(NULL);
  XML_SetUserData(p, parser);
  XML_SetElementHandler(p, xml_topology_parser_startElem, xml_topology_parser_endElem);
  parser -> topology = topology;

  /* parse! */
  do{
    /* read until ERROR or EOF */
    if(fgets(buff, XML_PARSER_BUFF_SIZE, fp) == NULL){
      done = 1;
      len = 0;
    }else{
      len = strlen(buff);
    }

    if((stat = XML_Parse(p, buff, len, done)) != XML_STATUS_OK){
      fprintf(stderr, "XML_Parse: error stat: %d\n", stat);
      exit(1);
    }

  }while(!done);
  
  XML_ParserFree(p);
  fclose(fp);

  /* return constructed xml topology */
  parser -> topology = NULL;
  return topology;
}

void
xml_topology_parser_test(xml_topology_parser_t parser, const char* filename){
  xml_top_node_vector_t path;
  float len, width;
  int i, j, size;
  const char *srcname;
  const char *dstname;
  
  xml_topology_parser_run(parser, filename);

  xml_topology_print(parser -> topology, NULL, 0);

  /* to all-to-all path traversal */
  size = xml_top_node_vector_size(parser -> topology -> nodes);
  for(i = 0; i < size; i++){
    srcname = xml_top_node_vector_get(parser -> topology -> nodes, i) -> name;
    for(j = i + 1; j < size; j++){
      dstname = xml_top_node_vector_get(parser -> topology -> nodes, j) -> name;

      path = xml_top_node_vector_create(2);
      assert(xml_topology_traverse(parser -> topology, srcname, dstname, path, &len, &width) == 1);
      printf("len:%.3f width:%.3f :", len, width);xml_top_node_vector_print(path, xml_top_node_print);
      xml_top_node_vector_destroy(path);
    }
  }
}
