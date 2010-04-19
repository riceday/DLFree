#include <string.h>
#include <netinet/tcp.h>
#include <sys/select.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
//#include <linux/tcp.h>

#include <std/std.h>
#include <iface/iface.h>
#include "impl/sock.h"

sock_t
base_sock_create(int fd, inet_iface_t dst_iface, int port){
  sock_t sk = (sock_t) std_malloc(sizeof(sock));
  sk -> port = port;
  sk -> dst_iface = dst_iface;
  sk -> fd = fd;
  
  return sk;
}

void
sock_destroy(sock_t sk){
  int fd = sk -> fd;
  if(fd >= 0){
    sk -> fd = -1;
    std_close(fd);
  }
  
  inet_iface_destroy(sk -> dst_iface);
  std_free(sk);
}

int
sock_fileno(sock_t sk){
  return sk -> fd;
}

int
sock_port(sock_t sk){
  return sk -> port;
}

inet_iface_t
sock_iface(sock_t sk){
  return sk -> dst_iface;
}

struct tcp_info
sock_tcp_info(sock_t sk){ /* TODO: not portable code */
  struct tcp_info info;
  socklen_t len = sizeof(struct tcp_info);
  std_getsockopt(sk -> fd, SOL_TCP, TCP_INFO, &info, &len);

  return info; /* return struct copy */
}
			

void
sock_set_nonblocking(sock_t sk, const int on){
  int flag;
  if((flag = fcntl(sk -> fd, F_GETFL, 0)) == -1){
    perror("fcntl");
    exit(1);
  }

  if(on){ /* set flag */
    flag |= O_NONBLOCK;
  }else{ /* clear flag */
    flag ^= O_NONBLOCK;
  }
  
  if(fcntl(sk -> fd, F_SETFL, flag) == -1){
    perror("fcntl");
    exit(1);
  }
}

void
sock_print(sock_t sk){
  fprintf(stdout, "sock(%s, %d)", inet_iface_in_addr_str(sk -> dst_iface), sk -> port);
}

void
sock_print_err(sock_t sk){
  fprintf(stderr, "sock(%s, %d)", inet_iface_in_addr_str(sk -> dst_iface), sk -> port);
}

sock_t
listen_sock_create(int myport, int backlog){
  int sockfd;
  const int on = 1;
  struct sockaddr_in my_addr;
  socklen_t addrlen = sizeof(my_addr);

  inet_iface_t my_iface = inet_iface_create_by_str("0.0.0.0");

  memset(&my_addr, 0, sizeof(my_addr));
  my_addr.sin_family = AF_INET;
  my_addr.sin_addr.s_addr = INADDR_ANY;
  my_addr.sin_port = htons(myport);
  
  sockfd = std_socket(AF_INET, SOCK_STREAM, 0);

  std_setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
  std_setsockopt(sockfd, SOL_TCP, TCP_NODELAY, &on, sizeof(on));

  std_bind(sockfd, (struct sockaddr*)&my_addr, addrlen);

  /* get port that was actually bound */
  memset(&my_addr, 0, sizeof(my_addr));
  std_getsockname(sockfd, (struct sockaddr*)&my_addr, &addrlen);
  myport = ntohs(my_addr.sin_port);

  std_listen(sockfd, backlog);
  
  return base_sock_create(sockfd, my_iface, myport);
}

sock_t
connect_sock_create(const char* addr_str, int port){
  sock_t sk;
  inet_iface_t dst_iface = inet_iface_create_by_str(addr_str);
  const int on = 1;
  int sockfd = std_socket(AF_INET, SOCK_STREAM, 0);

  std_setsockopt(sockfd, SOL_TCP, TCP_NODELAY, &on, sizeof(on));

  sk = base_sock_create(sockfd, dst_iface, port);

  /* set O_NONBLOCK */
  sock_set_nonblocking(sk, 1);
  
  return sk;
}

sock_t
connected_sock_create(int sockfd, const struct in_addr *addr, int port){
  sock_t sk;
  inet_iface_t dst_iface = inet_iface_create(addr);
  sk = base_sock_create(sockfd, dst_iface, port);

  /* set O_NONBLOCK */
  sock_set_nonblocking(sk, 1);
  
  return sk;
}

int
sock_connect(sock_t sk){
  struct sockaddr_in dst_addr;

  memset(&dst_addr, 0, sizeof(dst_addr));
  dst_addr.sin_family = AF_INET;
  dst_addr.sin_addr = inet_iface_in_addr(sk -> dst_iface);
  dst_addr.sin_port = htons(sk -> port);

  if(connect(sk -> fd, (const struct sockaddr*)&dst_addr, sizeof(dst_addr)) == -1 && errno != EINPROGRESS){
    perror("connect");
    return SOCK_CONNECT_ERR;
  }

  return SOCK_CONNECT_OK;
}

int
sock_connect_status(sock_t sk){
  int valopt;
  socklen_t vallen = sizeof(int);
  std_getsockopt(sk -> fd, SOL_SOCKET, SO_ERROR, &valopt, &vallen);
  if(valopt == 0)
    return SOCK_CONNECT_OK;
  else{
    fprintf(stderr, "sock_connect_status: connect failure: %s (%s, %d)\n", strerror(valopt), inet_iface_in_addr_str(sk -> dst_iface), sk -> port);
    return SOCK_CONNECT_ERR;
  }
}

sock_t
sock_accept(sock_t sk){
  int dstfd;
  struct sockaddr_in peer;
  socklen_t peer_len = sizeof(peer);
  struct in_addr *peer_addr;
  int peer_port;

  dstfd = std_accept(sk -> fd, (struct sockaddr*) &peer, &peer_len);
  peer_addr = &peer.sin_addr;
  peer_port = ntohs(peer.sin_port);

  return connected_sock_create(dstfd, peer_addr, peer_port);
}

int
sock_try_recv_n(sock_t sk, void* buf, int n, int *nr){
  if((*nr = recv(sk -> fd, buf, n, 0)) == -1){
    if(errno == EAGAIN){
      return SOCK_RECV_EAGAIN;
    }else{
      perror("recv");
      return SOCK_RECV_ERR;
    }
  }
  if(*nr == 0){ // EOF
    return SOCK_RECV_EOF;
  }
  return SOCK_RECV_OK;
}

int
sock_recv_n(sock_t sk, void *buf, int len){
  int tot = 0;
  int n, stat;

  /* for select */
  fd_set readfd;
  
  do {
    stat = sock_try_recv_n(sk, buf + tot, len - tot, &n);
    switch(stat){
    case SOCK_RECV_OK:
      tot += n;
      //printf("read %d/%d\n", n, len);fflush(stdout);
      break;
    case SOCK_RECV_EAGAIN:
      /* select and wait */
      FD_ZERO(&readfd);
      FD_SET(sk -> fd, &readfd);
      std_select(sk -> fd + 1, &readfd, NULL, NULL, NULL);
      break;
    case SOCK_RECV_EOF:
      return stat;
    case SOCK_RECV_ERR:
      return stat;
    default:
      fprintf(stderr, "sock_recv_n: invalid SOCK STATE: %d\n", stat);
      exit(1);
    }
  } while(tot < len);
  
  return stat;
}

int
sock_try_send_n(sock_t sk, const void* buf, int n, int* nr){
  if((*nr = send(sk -> fd, buf, n, 0)) == -1){
    if(errno == EAGAIN){
      return SOCK_SEND_EAGAIN;
    }else{
      perror("send");
      return SOCK_SEND_ERR;
    }
  }
  return SOCK_SEND_OK;
}

int
sock_send_n(sock_t sk, const void* buf, int len){
  int tot = 0;
  int n, stat;

  /* for select */
  fd_set writefd;
  
  do {
    stat = sock_try_send_n(sk, buf + tot, len - tot, &n);
    switch(stat){
    case SOCK_SEND_OK:
      tot += n;
      break;
    case SOCK_SEND_EAGAIN:
      /* select and wait */
      FD_ZERO(&writefd);
      FD_SET(sk -> fd, &writefd);
      std_select(sk -> fd + 1, NULL, &writefd, NULL, NULL);
      break;
    case SOCK_SEND_ERR:
      return stat;
    default:
      fprintf(stderr, "sock_send_n: invalid SOCK STATE: %d\n", stat);
      exit(1);
    }
  } while(tot < len);
  
  return stat;
}
