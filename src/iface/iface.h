#ifndef __IFACE_H__
#define __IFACE_H__

#include <netinet/in.h>

#define INET_IFACE_MAX_IFS (1024)

typedef struct inet_iface {
  struct in_addr in_addr;
  char *in_addr_str;
} inet_iface, *inet_iface_t;

inet_iface_t inet_iface_create(const struct in_addr* addr);
inet_iface_t inet_iface_create_by_str(const char* addr_str);
void inet_iface_destroy(inet_iface_t iface);
inet_iface_t inet_iface_dup(inet_iface_t iface);
struct in_addr inet_iface_in_addr(const inet_iface_t iface);
const char* inet_iface_in_addr_str(const inet_iface_t iface);
int inet_iface_is_same(const inet_iface_t x, const inet_iface_t y);

/* create inet_iface by scanning running inet addresses */
/* can specify preferred address priority with string array */
/* address matching to earlier string pattern get higher priority */
/* if no priority, an arbitrary inet address NOT 10.** 127.** is selected */
/* if no suitable address is found, NULL is returned  */
inet_iface_t inet_iface_get_my_iface(const char** prio_pat, int npat);
  
#endif // __IFACE_H__
