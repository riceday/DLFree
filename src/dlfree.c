#include <dlfree/dlfree.h>
#include <comm/comm.h>

/**
   Node communicator constructor
   \param node_id a unique integer node id
*/
dlfree_comm_node_t
dlfree_comm_node_create(int node_id){
  return comm_node_create(node_id);
}

/**
   Node communicator destructor
   \param node node communicator
*/
void
dlfree_comm_node_destroy(dlfree_comm_node_t node){
  comm_node_destroy(node);
}

/**
   Get the node communicator listen port
   \param node node communicator
   \return listen port
*/
int
dlfree_comm_node_listen_port(dlfree_comm_node_t node){
  return comm_node_listen_port(node);
}

/**
   Asynchronously dispatch a connect request to a given endpoint (ip, port)
   \param node node communicator
   \param addr destination IP address
   \param port destination port
   
   \return handle that can be used to synchronously wait for completion
*/
unsigned
long dlfree_comm_node_async_connect(dlfree_comm_node_t node, const char* addr, int port){
  return comm_node_async_connect(node, addr, port);
}

/**
   Synchronously wait for a dispatched connection request
   \param node   node communicator
   \param handle the handle returned when the request was dispatched
   \param dst_id the destination node communicator id, on success
   
   \return non-zero on failure
*/
int
dlfree_comm_node_connect_wait(dlfree_comm_node_t node, unsigned long handle, int* dst_id){
  return comm_node_connect_wait(node, handle, dst_id);
}

/**
   returns the RTT (Round Trip Time) with another node communicator
   \param node   node communicator
   \param dst_id the destination node communicator id
   
   \return the RTT in microseconds
*/
double
dlfree_comm_node_peer_rtt(dlfree_comm_node_t node, int dst_id){
  return comm_node_peer_rtt(node, dst_id);
}

/**
   Synchronously send a message to another node communicator.
   Internally, the message is segmented into 'message chunks' and queued on the
   internal finite queue. The function unblocks when all chunks are enqueued.
   
   \param node     node communicator
   \param dst_id   the destination node communicator id
   \param buff     pointer to the head of the data
   \param buffsize size of the buffer
*/
void
dlfree_comm_node_send_data(dlfree_comm_node_t node, int dst_id, const void *buff, int buffsize){
  comm_node_send_data(node, dst_id, buff, buffsize);
}

/**
   Synchronously wait for a message from a specific node communicator.
   The function unblocks if a message from a given node is fully received.
   
   \param node     node communicator
   \param src_id   the source node communicator id
   \param buff     pointer to the head of the data. The invoker must take ownership of the data.
   \param buffsize size of the message
*/
void
dlfree_comm_node_recv_data(dlfree_comm_node_t node, int src_id, void **buff, int* buffsize){
  comm_node_recv_data(node, src_id, buff, buffsize);
}

/**
   Synchronously wait for a message from any node communicator.
   The function unblocks if a message from any node is fully received.
   
   \param node     node communicator
   \param src_id   the source node communicator id from the data is received
   \param buff     pointer to the head of the data. The invoker must take ownership of the data.
   \param buffsize size of the message
*/
void
dlfree_comm_node_recv_any_data(dlfree_comm_node_t node, int* src_id, void **buff, int* buffsize){
  comm_node_recv_any_data(node, src_id, buff, buffsize);
}

