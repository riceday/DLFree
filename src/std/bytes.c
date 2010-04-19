#include <sys/types.h>
#include <arpa/inet.h>
#include "std.h"
#include "bytes.h"

uint64_t htonll(uint64_t v){
#if __BYTE_ORDER == __BIG_ENDIAN
  return v;
#else
  return htonl(v >> 32) + ((uint64_t)htonl(v) << 32);
#endif
}

uint64_t ntohll(uint64_t v){
#if __BYTE_ORDER == __BIG_ENDIAN
  return v;
#else
  return htonl(v >> 32) + (((uint64_t)htonl(v)) << 32);
#endif
}

void
pack_int(void **pp, int i){
  i = htonl(i);
  std_memcpy(*pp, &i, sizeof(int));
  *pp += sizeof(int);
}

int
unpack_int(const void **pp){
  int i = ntohl(*((int*)(*pp)));
  *pp += sizeof(int);
  return i;
}

void
pack_uint64(void **pp, uint64_t val){
  val = htonll(val);
  std_memcpy(*pp, &val, sizeof(uint64_t));
  *pp += sizeof(uint64_t);
}

uint64_t
unpack_uint64(const void **pp){
  int val = ntohll(*((uint64_t*)(*pp)));
  *pp += sizeof(uint64_t);
  return val;
}

void
pack_buff(void **pp, const void* src, int len){
  std_memcpy(*pp, src, len);
  *pp += len;
}
