#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h> // ioctl()
#include <net/if.h> // struct ifreq, ifconf
#include <netinet/in.h> // struct sockaddr, sockaddr_in

#include <std/std.h>
#include "iface.h"

inet_iface_t
inet_iface_create(const struct in_addr *addr){
  char *addr_str;
  inet_iface_t iface = (inet_iface_t)std_malloc(sizeof(inet_iface));

  std_memcpy(&iface -> in_addr, addr, sizeof(struct in_addr));

  addr_str = std_inet_ntoa(*addr);
  iface -> in_addr_str = strdup(addr_str);

  return iface;
}

inet_iface_t
inet_iface_create_by_str(const char* in_addr_str){
  inet_iface_t iface = (inet_iface_t)std_malloc(sizeof(inet_iface));

  iface -> in_addr_str = strdup(in_addr_str);
  std_inet_aton(iface -> in_addr_str, &iface -> in_addr);

  return iface;
}

void
inet_iface_destroy(inet_iface_t iface){
  std_free(iface -> in_addr_str);
  std_free(iface);
}

inet_iface_t
inet_iface_dup(const inet_iface_t iface){
  return inet_iface_create(&iface -> in_addr);
}

struct in_addr
inet_iface_in_addr(const inet_iface_t iface){
  return iface -> in_addr;
}

const char*
inet_iface_in_addr_str(const inet_iface_t iface){
  return iface -> in_addr_str;
}

int
inet_iface_is_same(const inet_iface_t x, const inet_iface_t y){
  // basically, comparing integers
  return (x -> in_addr.s_addr == y -> in_addr.s_addr);
}


inet_iface_t
inet_iface_get_my_iface(const char** prio_pats, int npat){
  int i;
  int sockfd;
  const char *ip;
  const char *pat;

  struct ifreq *ifrp;
  struct ifconf ifc;
  struct sockaddr_in *addr;
  char buf[sizeof(struct ifreq) * INET_IFACE_MAX_IFS];

  sockfd = std_socket(AF_INET, SOCK_STREAM, 0);

  /* need to set user buffer and size prior to ioctl */
  /* must make sure that enough buffer size is allocated */
  ifc.ifc_len = sizeof(buf);
  ifc.ifc_buf = (char*) buf;

  /* get 'all current L3 interface addresses that are running' */
  /* ifc_buf is written and ifc_len is set */
  if(ioctl(sockfd, SIOCGIFCONF, &ifc) == -1){
    perror("ioctl");
    exit(1);
  }
  std_close(sockfd);

  if(ifc.ifc_len == sizeof(buf)){ /* overflow is not an error */
    fprintf(stderr, "ifc.ifc_buf overflow\n");
    exit(1);
  }

  /* first, respect user specified priorities */
  for(i = 0; i < npat && prio_pats != NULL; i++){
    pat = prio_pats[i];
    /* simply iterate over all iface addresses that were set */
    for(ifrp = ifc.ifc_req; (char*)ifrp < (char*)ifc.ifc_buf + ifc.ifc_len; ifrp ++){
      /* if INET */
      if(ifrp -> ifr_addr.sa_family == AF_INET){
	addr = (struct sockaddr_in*)&(ifrp -> ifr_addr);
	ip = std_inet_ntoa(addr -> sin_addr);
	
	if(strncmp(ip, pat, strlen(pat)) == 0)
	  return inet_iface_create(&addr -> sin_addr);
      }
    }
  }

  /* by default, ignore 10.** and 127.** addresses TODO: ad-hoc */
  for(ifrp = ifc.ifc_req; (char*)ifrp < (char*)ifc.ifc_buf + ifc.ifc_len; ifrp ++){
    /* if INET */
    if(ifrp -> ifr_addr.sa_family == AF_INET){
      addr = (struct sockaddr_in*)&(ifrp -> ifr_addr);
      ip = std_inet_ntoa(addr -> sin_addr);
      
      if(strncmp("10.", ip, 3) && strncmp("127.", ip, 4))
	return inet_iface_create(&addr -> sin_addr);
    }
  }
  
  return NULL;
}
