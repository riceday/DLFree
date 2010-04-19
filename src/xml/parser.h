#ifndef __XML_PARSER_H__
#define __XML_PARSER_H__

#include <std/std.h>
#include <std/vector.h>
#include "topology.h"

#define XML_PARSER_BUFF_SIZE 1024

#define XML_CLUSTER_ELEMENT "CLUSTER"
#define XML_SWITCH_ELEMENT "SWITCH"
#define XML_NODE_ELEMENT "NODE"

#define XML_SWITCH_NAME_PREFIX "SW"
#define XML_ROOT_NAME_PREFIX "ROOT"

typedef struct xml_topology_parser{
  int switch_id;
  xml_top_node_vector_t node_stack;
  xml_topology_t topology;
} xml_topology_parser, *xml_topology_parser_t;

xml_topology_parser_t xml_topology_parser_create();
void xml_topology_parser_destroy(xml_topology_parser_t parser);
xml_topology_t xml_topology_parser_run(xml_topology_parser_t parser, const char* filename);
#endif // __XML_PARSER_H__
