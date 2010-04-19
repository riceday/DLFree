#ifndef __COLLECTIVE_H__
#define __COLLECTIVE_H__

#include <dlfree/dlfree.h>
#include <dlfree/gxp.h>

void send_all2one(gxp_man_t man, dlfree_comm_node_t comm_node, int dst_idx, long len, int iter);
void send_one2all(gxp_man_t man, dlfree_comm_node_t comm_node, int src_idx, long len, int iter);
void send_all2all(gxp_man_t man, dlfree_comm_node_t comm_node, long len, int iter);

#endif // __COLLECTIVE_H__
