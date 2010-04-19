#ifndef __DLFREE_COMM_H__
#define __DLFREE_COMM_H__

typedef void* dlfree_comm_node_t;

dlfree_comm_node_t dlfree_comm_node_create(int node_id);
void dlfree_comm_node_destroy(dlfree_comm_node_t node);
int dlfree_comm_node_listen_port(dlfree_comm_node_t node);
unsigned long dlfree_comm_node_async_connect(dlfree_comm_node_t node, const char* addr, int port);
int dlfree_comm_node_connect_wait(dlfree_comm_node_t node, unsigned long handle, int* dst_id);
double dlfree_comm_node_peer_rtt(dlfree_comm_node_t node, int dst_id);
void dlfree_comm_node_send_data(dlfree_comm_node_t node, int dst_id, const void *buff, int buffsize);
void dlfree_comm_node_recv_data(dlfree_comm_node_t node, int src_id, void **buff, int* buffsize);
void dlfree_comm_node_recv_any_data(dlfree_comm_node_t node, int* src_id, void **buff, int* buffsize);

#endif // __DLFREE_COMM_H__
