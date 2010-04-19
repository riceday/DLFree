#ifndef __BYTES_H__
#define __BYTES_H__

#include <sys/types.h>

void pack_int(void **pp, int i);
int unpack_int(const void **pp);
void pack_uint64(void **pp, uint64_t val);
uint64_t unpack_uint64(const void **pp);
void pack_buff(void **pp, const void* src, int len);

#endif // __BYTES_H__
