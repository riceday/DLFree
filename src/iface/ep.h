#ifndef __INET_EP_H__
#define __INET_EP_H__
#include "iface.h"

typedef struct inet_ep{
  inet_iface_t iface;
  int port;
} inet_ep, *inet_ep_t;

inet_ep_t inet_ep_create(const struct in_addr *addr, int port);
inet_ep_t inet_ep_create_by_str(const char *addr_str, int port);
void inet_ep_destroy(inet_ep_t ep);
void inet_ep_print(inet_ep_t ep);
inet_iface_t inet_ep_iface(inet_ep_t ep);
int inet_ep_port(inet_ep_t ep);
int inet_ep_is_same(const inet_ep_t x, const inet_ep_t y);

#endif // __INET_EP_H__
