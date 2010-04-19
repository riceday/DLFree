#ifndef __COMM_H__
#define __COMM_H__

typedef struct comm_node comm_node, *comm_node_t;

comm_node_t comm_node_create(int node_id);
void comm_node_destroy(comm_node_t node);
int comm_node_listen_port(comm_node_t node);
unsigned long comm_node_async_connect(comm_node_t node, const char* addr, int port);
int comm_node_connect_wait(comm_node_t node, unsigned long handle, int* dst_id);
double comm_node_peer_rtt(comm_node_t node, int dst_id);
void comm_node_send_data(comm_node_t node, int dst_id, const void *buff, int buffsize);
void comm_node_recv_data(comm_node_t node, int src_id, void **buff, int* buffsize);
void comm_node_recv_any_data(comm_node_t node, int* src_id, void **buff, int* buffsize);

#endif // __COMM_H__
