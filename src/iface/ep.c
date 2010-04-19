#include <netinet/in.h>

#include <std/std.h>
#include "ep.h"

inet_ep_t
inet_ep_create(const struct in_addr *addr, int port){
  inet_ep_t ep;
  ep = (inet_ep_t) std_malloc(sizeof(inet_ep));
  ep -> iface = inet_iface_create(addr);
  ep -> port = port;

  return ep;
}

inet_ep_t
inet_ep_create_by_str(const char *addr_str, int port){
  inet_ep_t ep;
  ep = (inet_ep_t) std_malloc(sizeof(inet_ep));
  ep -> iface = inet_iface_create_by_str(addr_str);
  ep -> port = port;

  return ep;
}

void
inet_ep_destroy(inet_ep_t ep){
  inet_iface_destroy(ep -> iface);
  std_free(ep);
}

void
inet_ep_print(inet_ep_t ep){
  const char* addr_str= inet_iface_in_addr_str(ep -> iface);
  fprintf(stdout, "ep(%s, %d)", addr_str, ep -> port);
}

inet_iface_t
inet_ep_iface(inet_ep_t ep){
  return ep -> iface;
}

int
inet_ep_port(inet_ep_t ep){
  return ep -> port;
}

int
inet_ep_is_same(const inet_ep_t x, const inet_ep_t y){
  return inet_iface_is_same(x -> iface, y -> iface) && x -> port == y -> port;
}

