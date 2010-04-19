#ifndef __IMPL_SOCK_H__
#define __IMPL_SOCK_H__

#include <iface/iface.h>

enum connect_status{
  SOCK_CONNECT_OK,
  SOCK_CONNECT_ERR,
};

enum send_status{
  SOCK_SEND_OK,
  SOCK_SEND_EAGAIN,
  SOCK_SEND_ERR,
};

enum recv_status{
  SOCK_RECV_OK,
  SOCK_RECV_EAGAIN,
  SOCK_RECV_EOF,
  SOCK_RECV_ERR,
};

typedef struct sock{
  int fd;
  inet_iface_t dst_iface;
  int port;

} sock, *sock_t;

sock_t base_sock_create(int fd, inet_iface_t dst_iface, int port);
void sock_destroy(sock_t sk);
int sock_fileno(sock_t sk);
int sock_port(sock_t sk);
inet_iface_t sock_iface(sock_t sk);
struct tcp_info sock_tcp_info(sock_t sk);
void sock_print(sock_t sk);
void sock_print_err(sock_t sk);
sock_t listen_sock_create(int myport, int backlog);
sock_t connect_sock_create(const char* addr_str, int port);
sock_t connected_sock_create(int sockfd, const struct in_addr *addr, int port);
int sock_connect(sock_t sk);
int sock_connect_status(sock_t sk);
sock_t sock_accept(sock_t sk);
int sock_try_recv_n(sock_t sk, void* buf, int n, int *nr);
int sock_recv_n(sock_t sk, void* buf, int n);
int sock_try_send_n(sock_t sk, const void* buf, int n, int *nr);
int sock_send_n(sock_t sk, const void* buf, int n);

#endif // __IMPL_SOCK_H__
